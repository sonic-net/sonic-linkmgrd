#!/usr/bin/env python3
"""Prove the finalized C++ database contains Linkmgrd's actual compiled definitions."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
import zipfile


QUERY = """import cpp
from Function f, File source
where source = f.getDefinition().getFile() and
  source.getAbsolutePath().matches("%/src/%.cpp")
select f.getName(), source.getAbsolutePath()
"""


def checkout_identity(workspace, head, base):
    git = ["git", "-c", "safe.directory=" + str(workspace)]
    def call(*args):
        return subprocess.check_output(git + list(args), cwd=workspace, text=True).strip()
    assert not call("status", "--porcelain"), "Source checkout is not clean"
    revision = call("rev-parse", "HEAD")
    parents = [line.removeprefix("parent ") for line in call("cat-file", "-p", "HEAD").split("\n\n", 1)[0].splitlines() if line.startswith("parent ")]
    if head and head != revision:
        assert head in parents and base and base in parents
    return {"revision": revision, "parents": parents, "requested_head": head, "requested_base": base}


def verify_owner(rows, source_zip, workspace):
    expected = {str(path.relative_to(workspace)): path for path in (workspace / "src").rglob("*.cpp")}
    checked = {}
    with zipfile.ZipFile(source_zip) as archive:
        names = set(archive.namelist())
        for relative, original in expected.items():
            definitions = {(name, path) for name, path in rows if path.endswith("/" + relative)}
            if not definitions:
                raise AssertionError("No extracted owner definition in " + relative)
            source = original.read_bytes()
            for _, path in definitions:
                member = path.lstrip("/")
                if member not in names or archive.read(member) != source:
                    raise AssertionError("Extracted source bytes differ from checkout: " + relative)
            checked[relative] = {"sha256": hashlib.sha256(source).hexdigest(), "functions": sorted({name for name, _ in definitions})}
    assert expected and "main" in checked["src/LinkMgrdMain.cpp"]["functions"]
    return checked


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--codeql", required=True, type=Path)
    parser.add_argument("--database", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    workspace = Path(__file__).resolve().parents[2]
    identity = checkout_identity(workspace, os.environ.get("CI_HEAD_SHA", ""), os.environ.get("CI_BASE_SHA", ""))
    cli = args.codeql.resolve()
    if not cli.is_file():
        raise AssertionError("The initialized CodeQL CLI does not exist")
    version = json.loads(subprocess.check_output([str(cli), "version", "--format=json"], text=True))
    if version.get("version") != "2.27.0":
        raise AssertionError("CodeQL differs from the workflow's pinned bundle")
    bundle = cli.parent
    packs = json.loads(subprocess.check_output([str(cli), "resolve", "qlpacks", "--format=json"], text=True))
    candidates = [Path(path).resolve() for path in packs.get("codeql/cpp-all", [])]
    candidates = [path for path in candidates if path.is_relative_to(bundle)]
    versions = {re.search(r"^version: (\S+)$", (path / "qlpack.yml").read_text(), re.M).group(1)
                for path in candidates}
    if versions != {"12.1.0"}:
        raise AssertionError("Expected cpp-all 12.1.0 from the immutable CodeQL 2.27.0 bundle")
    with tempfile.TemporaryDirectory(prefix="linkmgrd-codeql-query-") as temporary:
        root = Path(temporary)
        (root / "qlpack.yml").write_text("name: sonic/linkmgrd-coverage\nversion: 0.0.0\ndependencies:\n  codeql/cpp-all: 12.1.0\n")
        query = root / "owner.ql"
        query.write_text(QUERY)
        result = root / "owner.bqrs"
        subprocess.run([str(cli), "query", "run", "--database=" + str(args.database),
                        "--search-path=" + str(bundle), "--threads=2", "--ram=2048",
                        "--output=" + str(result), str(query)], check=True)
        decoded = json.loads(subprocess.check_output([str(cli), "bqrs", "decode", "--format=json", str(result)], text=True))
        checked = verify_owner(decoded["#select"]["tuples"], args.database / "src.zip", workspace)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps({"status": "passed", "codeql": version["version"],
                                      "cpp_all": "12.1.0", "source_identity": identity, "owner_sources": checked}, indent=2) + "\n")


if __name__ == "__main__":
    main()
