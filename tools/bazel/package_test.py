"""Exercise the installed Linkmgrd ELF with declared native runtime inputs."""

import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tarfile
import tempfile
import unittest
import zlib

import elf_metadata
from elf_metadata import Elf


RUNTIME, DEBUG, CHANGELOG, RUNNER, *PAYLOADS = [Path(p).resolve() for p in sys.argv[1:]]
sys.argv[1:] = []


def unpack(path, destination):
    with tarfile.open(path) as archive:
        members = archive.getmembers()
        names = [m.name.removeprefix('./') for m in members]
        if len(names) != len(set(names)):
            raise AssertionError('Duplicate installed path')
        for member in members:
            if member.isdir():
                continue
            if not member.isfile():
                raise AssertionError(f'Unexpected package member type: {member.name}')
            assert member.uid == member.gid == 0, member.name
            assert member.mtime == 1672560000, (member.name, member.mtime)
            archive.extract(member, destination, filter='data')
        return {m.name.removeprefix('./'): m for m in members if m.isfile()}


class PackageTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix='linkmgrd-installed-')
        cls.root = Path(cls.temporary.name)
        cls.runtime = unpack(RUNTIME, cls.root)
        cls.debug = unpack(DEBUG, cls.root)
        cls.binary = cls.root / 'usr/sbin/linkmgrd'
        cls.elf = Elf(cls.binary.read_bytes())
        cls.receipt = {
            'runtime_sha256': hashlib.sha256(RUNTIME.read_bytes()).hexdigest(),
            'debug_sha256': hashlib.sha256(DEBUG.read_bytes()).hexdigest(),
        }

    @classmethod
    def tearDownClass(cls):
        cls.temporary.cleanup()

    def test_installed_contract_and_symbols(self):
        self.assertEqual(set(self.runtime), {
            'usr/sbin/linkmgrd',
            'usr/share/doc/sonic-linkmgrd/changelog.Debian.gz',
        })
        self.assertEqual(self.runtime['usr/sbin/linkmgrd'].mode, 0o755)
        doc = 'usr/share/doc/sonic-linkmgrd/changelog.Debian.gz'
        self.assertEqual(self.runtime[doc].mode, 0o644)
        self.assertEqual(gzip.decompress((self.root / doc).read_bytes()), CHANGELOG.read_bytes())
        self.assertEqual(self.elf.machine, 62)
        self.assertEqual(self.elf.bits, 64)
        for soname in ('libswsscommon.so.0', 'libnl-3.so.200', 'libnl-route-3.so.200'):
            with self.subTest(required_shared_library=soname):
                self.assertIn(soname, self.elf.needed)
        self.assertNotIn('.debug_info', self.elf.by_name)
        self.assertIsNotNone(self.elf.build_id)
        expected = 'usr/lib/debug/.build-id/' + self.elf.build_id[:2] + '/' + self.elf.build_id[2:] + '.debug'
        self.assertEqual(set(self.debug), {expected})
        self.assertEqual(self.debug[expected].mode, 0o644)
        symbols = (self.root / expected).read_bytes()
        self.assertEqual(Elf(symbols).build_id, self.elf.build_id)
        self.assertIn('.debug_info', Elf(symbols).by_name)
        self.assertEqual(zlib.crc32(symbols), self.elf.debuglink_crc)
        for section in self.elf.sections:
            if section[1] == 6:
                for offset in range(section[4], section[4] + section[5], section[9]):
                    tag, _ = self.elf.unpack('qQ', offset)
                    self.assertNotIn(tag, (15, 29), 'Installed ELF retained RPATH/RUNPATH')
        self.receipt.update(build_id=self.elf.build_id, needed=self.elf.needed)
        environment = dict(os.environ, DEBUGINFOD_URLS='')
        def inspect(symbol_directory):
            return subprocess.run([
                'gdb', '-nx', '-batch', '-ex', 'set debuginfod enabled off',
                '-ex', 'set auto-load safe-path /nonexistent',
                '-ex', 'set debug-file-directory ' + str(symbol_directory),
                '-ex', 'file ' + str(self.binary), '-ex', 'info line main',
                '-ex', 'ptype mux::MuxManager',
            ], env=environment, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        negative = inspect(self.root / 'no-symbols')
        self.assertNotRegex(negative.stdout, r'Line \d+ of .*LinkMgrdMain.cpp')
        self.assertNotIn('type = class mux::MuxManager', negative.stdout)
        positive = inspect(self.root / 'usr/lib/debug')
        self.assertEqual(positive.returncode, 0, positive.stdout)
        self.assertRegex(positive.stdout, r'Line \d+ of .*LinkMgrdMain.cpp')
        self.assertIn('type = class mux::MuxManager', positive.stdout)
        self.receipt['gdb'] = {'with_symbols': positive.stdout, 'without_symbols': negative.stdout}

    def run_cli(self, *arguments):
        environment = dict(os.environ)
        environment['PYTHONPATH'] = str(Path(elf_metadata.__file__).parent)
        command = [sys.executable, str(RUNNER), '--binary', str(self.binary)]
        for payload in PAYLOADS:
            command += ['--payload', str(payload)]
        return subprocess.run(command + ['--', *arguments], cwd=self.root,
                              env=environment, text=True, capture_output=True, timeout=60)

    def test_real_installed_help(self):
        result = self.run_cli('--help')
        # Preserve the original CLI: help intentionally returns EXIT_FAILURE.
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        for option in ('linkmgrd options', '--verbosity', '--default_route', '--link_to_swss_logger'):
            self.assertIn(option, result.stdout)
        self.receipt['help'] = {'exit_code': result.returncode, 'stdout': result.stdout}

    def test_real_installed_invalid_option(self):
        result = self.run_cli('--sonic-invalid-option')
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertIn('Command Line Syntax Error', result.stderr)
        self.assertIn('sonic-invalid-option', result.stderr)
        self.receipt['invalid_option'] = {'exit_code': result.returncode, 'stderr': result.stderr}


if __name__ == '__main__':
    result = unittest.main(exit=False).result
    output = Path(os.environ.get('TEST_UNDECLARED_OUTPUTS_DIR', '.'))
    output.mkdir(parents=True, exist_ok=True)
    (output / 'installed.json').write_text(json.dumps({
        'status': 'passed' if result.wasSuccessful() else 'failed',
        'tests': result.testsRun, **PackageTest.receipt,
    }, indent=2) + '\n')
    raise SystemExit(not result.wasSuccessful())
