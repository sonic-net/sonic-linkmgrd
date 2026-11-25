"""Retain native source, installed-package and incremental build evidence."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shutil
import subprocess
import time
from urllib.parse import unquote, urlparse
import zipfile

from defusedxml import ElementTree as ET


ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'bazel-evidence'
BAZEL = os.environ.get('BAZEL_BINARY', 'bazel')
BUILDS = ['//:sonic_linkmgrd_runtime', '//:sonic_linkmgrd_debug', '//:runtime_packages.json']
TESTS = ['//:linkmgrd_test', '//:source_contract_test', '//:package_test']


def checkout_identity():
    git = ['git', '-c', 'safe.directory=' + str(ROOT)]
    def call(*args):
        return subprocess.check_output(git + list(args), cwd=ROOT, text=True).strip()
    assert not call('status', '--porcelain'), 'Acceptance requires committed, clean source inputs'
    revision = call('rev-parse', 'HEAD')
    parents = [line.removeprefix('parent ') for line in call('cat-file', '-p', 'HEAD').split('\n\n', 1)[0].splitlines()
               if line.startswith('parent ')]
    head, base = os.environ.get('CI_HEAD_SHA', ''), os.environ.get('CI_BASE_SHA', '')
    if head and head != revision:
        assert head in parents and base and base in parents, (revision, parents, head, base)
    return {'revision': revision, 'parents': parents, 'requested_head': head, 'requested_base': base}


def test_reports():
    reports, summaries, seen = {}, set(), set()
    for line in (OUT / 'test-events.jsonl').read_text().splitlines():
        event = json.loads(line)
        label = event.get('id', {}).get('testResult', {}).get('label')
        if label in TESTS:
            assert label not in seen, 'Unexpected repeated test attempt: ' + label
            seen.add(label)
            result = event['testResult']
            assert result['status'] == 'PASSED', (label, result)
            for file in result.get('testActionOutput', []):
                if file['name'] == 'test.xml':
                    uri = urlparse(file['uri'])
                    assert uri.scheme == 'file' and not uri.netloc, uri
                    path = Path(unquote(uri.path))
                    assert path.is_file(), path
                    reports[label] = path
        label = event.get('id', {}).get('testSummary', {}).get('label')
        if label in TESTS:
            assert event['testSummary']['overallStatus'] == 'PASSED', event
            summaries.add(label)
    assert set(reports) == summaries == set(TESTS), (reports, summaries)
    return reports


def run(*args, output=None):
    command = [BAZEL, *args]
    if output:
        with output.open('w') as stream:
            subprocess.run(command, cwd=ROOT, stdout=stream, stderr=subprocess.STDOUT, check=True)
    else:
        subprocess.run(command, cwd=ROOT, check=True)


def capture(*args):
    return subprocess.check_output([BAZEL, *args], cwd=ROOT, text=True)


def paths():
    result = {}
    for target in BUILDS:
        files = capture('cquery', target, '--output=files').splitlines()
        assert len(files) == 1, (target, files)
        result[target] = ROOT / files[0]
    return result


def hashes(files):
    return {target: hashlib.sha256(path.read_bytes()).hexdigest() for target, path in files.items()}


def audit():
    (OUT / 'action-audit.json').write_text('{"status":"running"}\n')
    query = 'deps(set(' + ' '.join(BUILDS + TESTS) + '))'
    with (OUT / 'actions.json').open('w') as stream:
        subprocess.run([BAZEL, 'aquery', query, '--output=jsonproto'], cwd=ROOT, stdout=stream, check=True)
    graph = json.loads((OUT / 'actions.json').read_text())
    fragments = {x['id']: x for x in graph['pathFragments']}
    def path(identifier):
        parts = []
        while identifier:
            part = fragments[identifier]
            parts.append(part['label'])
            identifier = part.get('parentId', 0)
        return '/'.join(reversed(parts))
    artifacts = {x['id']: path(x['pathFragmentId']) for x in graph['artifacts']}
    outputs = {artifacts[i] for a in graph['actions'] for i in a.get('outputIds', [])}
    assert outputs
    assert not any(p.endswith(('.deb', '.udeb', '.ddeb')) for p in outputs)
    for action in graph['actions']:
        command = action.get('arguments', [])
        if any(Path(arg).name == 'dpkg-deb' for arg in command):
            assert '--build' not in command and '-b' not in command, command
    (OUT / 'action-audit.json').write_text(json.dumps({
        'status': 'passed', 'actions': len(graph['actions']),
        'outputs': len(outputs), 'deb_outputs': 0, 'targets': BUILDS + TESTS,
    }, indent=2) + '\n')


def build():
    (OUT / 'build.json').write_text('{"status":"running"}\n')
    run('build', '--jobs=2', '--local_resources=cpu=2', *BUILDS)
    run('test', '--jobs=2', '--local_resources=cpu=2', '--local_test_jobs=1',
        '--build_event_json_file=' + str(OUT / 'test-events.jsonl'), *TESTS)
    test_reports()
    (OUT / 'build.json').write_text(json.dumps({
        'status': 'passed', 'identity': checkout_identity(), 'outputs': hashes(paths()),
        'events_sha256': hashlib.sha256((OUT / 'test-events.jsonl').read_bytes()).hexdigest(),
    }, indent=2) + '\n')


def incremental():
    (OUT / 'incremental.json').write_text('{"status":"running"}\n')
    source = ROOT / 'src/LinkMgrdMain.cpp'
    original = source.read_bytes()
    marker = b'"linkmgrd options"'
    assert original.count(marker) == 1
    edited = original.replace(marker, b'"linkmgrd incremental probe options"')
    results, files = {}, None
    def step(name):
        nonlocal files
        begin = time.monotonic()
        run('build', '--jobs=2', '--local_resources=cpu=2', *BUILDS,
            output=OUT / ('incremental-' + name + '.log'))
        elapsed = time.monotonic() - begin
        files = files or paths()
        results[name] = {'seconds': elapsed, 'sha256': hashes(files)}
        return results[name]['sha256']
    try:
        baseline = step('baseline')
        assert step('noop') == baseline
        assert source.read_bytes() == original, 'Concurrent source change'
        source.write_bytes(edited)
        changed = step('edit')
        assert all(changed[target] != baseline[target] for target in BUILDS[:2]), results
    finally:
        assert source.read_bytes() in (original, edited), 'Concurrent source change; refusing overwrite'
        source.write_bytes(original)
    assert step('restore') == baseline
    (OUT / 'incremental.json').write_text(json.dumps({
        'status': 'passed', 'source': str(source.relative_to(ROOT)), 'builds': results,
        'identity': checkout_identity(),
        'scope': 'Retained-cache baseline/no-op/source-edit/restore; not a cache-free build',
    }, indent=2) + '\n')


def collect():
    files = paths()
    identity = checkout_identity()
    build_receipt = json.loads((OUT / 'build.json').read_text())
    incremental_receipt = json.loads((OUT / 'incremental.json').read_text())
    assert build_receipt['status'] == incremental_receipt['status'] == 'passed'
    assert identity == build_receipt['identity'] == incremental_receipt['identity']
    assert hashes(files) == build_receipt['outputs'] == incremental_receipt['builds']['baseline']['sha256'] == incremental_receipt['builds']['restore']['sha256']
    assert hashlib.sha256((OUT / 'test-events.jsonl').read_bytes()).hexdigest() == build_receipt['events_sha256']
    reports = test_reports()
    outputs = {target: {'file': path.name, 'sha256': hashes({target: path})[target]}
               for target, path in files.items()}
    for path in files.values():
        shutil.copy2(path, OUT / path.name)
    summaries = {}
    for target in TESTS:
        name = target.removeprefix('//:')
        logs = reports[target].parent
        xml = ET.parse(reports[target]).getroot()
        cases = list(xml.iter('testcase'))
        assert cases and not list(xml.iter('failure')) and not list(xml.iter('error')), name
        skipped = len(list(xml.iter('skipped')))
        assert not skipped, (name, skipped)
        summaries[target] = {'tests': len(cases), 'skipped': skipped}
        shutil.copytree(logs, OUT / 'tests' / name, dirs_exist_ok=True)
    original_tests = {
        suite + '.' + name
        for source in (ROOT / 'test').glob('*.cpp')
        for suite, name in re.findall(r'\bTEST(?:_F)?\s*\(\s*(\w+)\s*,\s*(\w+)\s*\)', source.read_text())
    }
    owner_xml = ET.parse(reports['//:linkmgrd_test'])
    executed = {case.attrib['classname'] + '.' + case.attrib['name']
                for case in owner_xml.iter('testcase')}
    assert original_tests and original_tests <= executed, sorted(original_tests - executed)
    summaries['//:linkmgrd_test']['original_registered_tests'] = len(original_tests)
    parameterized = {
        (suite, name)
        for source in (ROOT / 'test').glob('*.cpp')
        for suite, name in re.findall(r'\bTEST_P\s*\(\s*(\w+)\s*,\s*(\w+)\s*\)', source.read_text())
    }
    for suite, name in parameterized:
        assert any(case.attrib['classname'].split('/')[-1] == suite and case.attrib['name'].split('/')[0] == name
                   for case in owner_xml.iter('testcase')), (suite, name)
    summaries['//:linkmgrd_test']['parameterized_definitions'] = len(parameterized)
    outputs_dir = reports['//:package_test'].parent / 'test.outputs'
    if (outputs_dir / 'installed.json').exists():
        installed = json.loads((outputs_dir / 'installed.json').read_text())
    else:
        with zipfile.ZipFile(outputs_dir / 'outputs.zip') as archive:
            installed = json.loads(archive.read('installed.json'))
    assert installed['status'] == 'passed' and installed['tests'] == 3
    assert installed['runtime_sha256'] == outputs[BUILDS[0]]['sha256']
    assert installed['debug_sha256'] == outputs[BUILDS[1]]['sha256']
    (OUT / 'installed.json').write_text(json.dumps(installed, indent=2) + '\n')
    git = ['git', '-c', 'safe.directory=' + str(ROOT)]
    def git_output(*args):
        return subprocess.check_output(git + list(args), cwd=ROOT, text=True).strip()
    assert not git_output('status', '--porcelain', '--untracked-files=normal'), 'Commit producer inputs before evidence collection'
    revision = git_output('rev-parse', 'HEAD')
    headers = git_output('cat-file', '-p', 'HEAD').split('\n\n', 1)[0]
    parents = [line.removeprefix('parent ') for line in headers.splitlines() if line.startswith('parent ')]
    head, base = os.environ.get('CI_HEAD_SHA', ''), os.environ.get('CI_BASE_SHA', '')
    if head:
        assert head == revision or head in parents, (revision, parents, head)
        if base and head != revision:
            assert base in parents, (base, parents)
    (OUT / 'resolved-modules.txt').write_text(capture('mod', 'graph'))
    shutil.copy2(ROOT / 'MODULE.bazel.lock', OUT / 'MODULE.bazel.lock')
    (OUT / 'provenance.json').write_text(json.dumps({
        'tested_revision': revision, 'parents': parents, 'pull_request_head': head,
        'pull_request_base': base, 'architecture': platform.machine(),
        'os_release': Path('/etc/os-release').read_text(),
        'outputs': outputs, 'tests': summaries,
        'scope': 'Native AMD64/Trixie, source-owned runtime/debug TARs and metadata; no DEB generation',
    }, indent=2) + '\n')


if __name__ == '__main__':
    commands = {'audit': audit, 'build': build, 'incremental': incremental, 'collect': collect}
    parser = argparse.ArgumentParser()
    parser.add_argument('command', choices=commands)
    args = parser.parse_args()
    OUT.mkdir(exist_ok=True)
    identity = checkout_identity()
    identity_file = OUT / 'source-identity.json'
    if args.command == 'audit':
        identity_file.write_text(json.dumps(identity, indent=2) + '\n')
    else:
        assert identity == json.loads(identity_file.read_text()), 'Source identity changed since action audit'
        assert json.loads((OUT / 'action-audit.json').read_text())['status'] == 'passed'
    commands[args.command]()
