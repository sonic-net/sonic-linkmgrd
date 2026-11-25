"""Keep the maintained source globs aligned with the original Make inputs."""

from pathlib import Path
import re
import unittest


class SourceContractTest(unittest.TestCase):
    def test_original_translation_units_are_preserved(self):
        root = Path(__file__).parents[2]
        declared = set()
        makefiles = [root / "src/subdir.mk", *sorted((root / "src").glob("*/subdir.mk")),
                     root / "test/subdir.mk"]
        for path in makefiles:
            section = path.read_text().split("CPP_SRCS +=", 1)[1].split("\n\n", 1)[0]
            declared.update(re.findall(r"\./((?:src|test)/[^\s\\]+\.cpp)", section))
        actual = {str(path.relative_to(root)) for directory in ("src", "test")
                  for path in (root / directory).rglob("*.cpp")}
        self.assertEqual(actual, declared)
        self.assertIn("src/LinkMgrdMain.cpp", declared)
        self.assertIn("test/LinkMgrdTestMain.cpp", declared)


if __name__ == "__main__":
    unittest.main()
