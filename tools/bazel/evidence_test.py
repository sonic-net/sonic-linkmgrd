"""Exercise source identity checks against a real shallow Git merge checkout."""
import importlib.util
import json
import sys
import zipfile
from pathlib import Path
import subprocess
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("linkmgrd_coverage", Path(__file__).with_name("codeql_coverage.py"))
validate = importlib.util.module_from_spec(spec)
spec.loader.exec_module(validate)

class SourceIdentityTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="wpa-identity-")
        self.addCleanup(self.temporary.cleanup)
        root = Path(self.temporary.name)
        origin = root / "origin"
        origin.mkdir()
        def git(where, *args, input=None):
            return subprocess.check_output(["git", "-c", "user.name=Guohan Lu", "-c", "user.email=securely1g@gmail.com", "-c", "commit.gpgSign=false", "-c", "core.hooksPath=/dev/null", "-C", str(where), *args], input=input, text=True, stderr=subprocess.PIPE).strip()
        git(origin, "init", "--initial-branch=main")
        (origin / "source.c").write_text("int baseline;\n")
        git(origin, "add", "source.c")
        git(origin, "commit", "-m", "baseline")
        self.base = git(origin, "rev-parse", "HEAD")
        git(origin, "switch", "-c", "feature")
        (origin / "source.c").write_text("int feature;\n")
        git(origin, "commit", "-am", "feature")
        self.head = git(origin, "rev-parse", "HEAD")
        self.merge = git(origin, "commit-tree", "HEAD^{tree}", "-p", self.base, "-p", self.head, input="tested merge\n")
        git(origin, "update-ref", "refs/heads/ci", self.merge)
        self.checkout = root / "checkout"
        git(root, "clone", "--depth=1", "--single-branch", "--branch=ci", origin.as_uri(), str(self.checkout))
        self.assertEqual(git(self.checkout, "show", "-s", "--format=%P", "HEAD"), "")

    def test_shallow_merge_preserves_both_actual_parent_identities(self):
        identity = validate.checkout_identity(self.checkout, self.head, self.base)
        self.assertEqual(identity["revision"], self.merge)
        self.assertEqual(identity["parents"], [self.base, self.head])

    def test_unrelated_head_and_base_are_rejected(self):
        for head, base in [("0" * 40, self.base), (self.head, "0" * 40)]:
            with self.subTest(head=head, base=base), self.assertRaises(AssertionError):
                validate.checkout_identity(self.checkout, head, base)

    def test_dirty_source_cannot_be_attributed_to_the_merge(self):
        (self.checkout / "unexpected.c").write_text("untracked source\n")
        with self.assertRaises(AssertionError):
            validate.checkout_identity(self.checkout, self.head, self.base)

class ExtractedSourceTest(unittest.TestCase):
    def test_missing_or_changed_source_is_rejected(self):
        import zipfile
        with tempfile.TemporaryDirectory(prefix="linkmgrd-extracted-") as temporary:
            root = Path(temporary)
            (root / "src").mkdir()
            source = b"int main() { return 0; }\n"
            (root / "src/LinkMgrdMain.cpp").write_bytes(source)
            archive = root / "src.zip"
            member = "checkout/src/LinkMgrdMain.cpp"
            with zipfile.ZipFile(archive, "w") as output:
                output.writestr(member, source)
            rows = [["main", "/" + member]]
            self.assertEqual(set(validate.verify_owner(rows, archive, root)), {"src/LinkMgrdMain.cpp"})
            with self.assertRaises(AssertionError):
                validate.verify_owner([], archive, root)
            (root / "src/LinkMgrdMain.cpp").write_bytes(b"int different;\n")
            with self.assertRaises(AssertionError):
                validate.verify_owner(rows, archive, root)



class BuildEventsTest(unittest.TestCase):
    def test_only_current_passing_events_supply_reports(self):
        import json
        spec = importlib.util.spec_from_file_location("linkmgrd_validate", Path(__file__).with_name("validate.py"))
        build = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(build)
        with tempfile.TemporaryDirectory(prefix="linkmgrd-events-") as temporary:
            root = Path(temporary)
            build.OUT = root
            rows = []
            for index, label in enumerate(build.TESTS):
                report = root / (str(index) + ".xml")
                report.write_text("<testsuite/>")
                rows.append({"id": {"testResult": {"label": label}}, "testResult": {
                    "status": "PASSED", "testActionOutput": [{"name": "test.xml", "uri": report.as_uri()}]}})
                rows.append({"id": {"testSummary": {"label": label}}, "testSummary": {"overallStatus": "PASSED"}})
            events = root / "test-events.jsonl"
            def write(items):
                events.write_text("\n".join(json.dumps(item) for item in items) + "\n")
            write(rows)
            self.assertEqual(set(build.test_reports()), set(build.TESTS))
            # Existing XML files cannot compensate for missing current results.
            write(rows[1:])
            with self.assertRaises(AssertionError):
                build.test_reports()
            write(rows + rows[:1])
            with self.assertRaises(AssertionError):
                build.test_reports()
            rows[0]["testResult"]["status"] = "FAILED"
            write(rows)
            with self.assertRaises(AssertionError):
                build.test_reports()


if __name__ == "__main__":
    unittest.main()
