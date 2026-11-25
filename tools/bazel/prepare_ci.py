"""Use the checked-in registries and preserve setup-bazel caches in CI."""

import os
from pathlib import Path
import shlex
import shutil
import tempfile


configuration = Path('.bazelrc').read_text()
home = Path.home() / '.bazelrc'
if home.is_file():
    caches = [line for line in home.read_text().splitlines()
              if line.startswith(('build --disk_cache=', 'common --repository_cache='))]
    configuration += '\n' + '\n'.join(caches) + '\n'
directory = Path(tempfile.mkdtemp(prefix='linkmgrd-bazel-'))
rc = directory / 'ci.bazelrc'
rc.write_text(configuration)
binary = shutil.which('bazel')
assert binary
wrapper = directory / 'bazel'
wrapper.write_text('#!/bin/sh\nexec ' + shlex.quote(binary) +
                   ' --host_jvm_args=-Xmx3g --nosystem_rc --nohome_rc --noworkspace_rc --bazelrc=' +
                   shlex.quote(str(rc)) + ' "$@"\n')
wrapper.chmod(0o755)
evidence = Path('bazel-evidence')
evidence.mkdir(exist_ok=True)
shutil.copyfile('.bazelrc', evidence / 'registry.bazelrc')
if os.environ.get('GITHUB_ENV'):
    with open(os.environ['GITHUB_ENV'], 'a') as environment:
        environment.write('BAZEL_BINARY=' + str(wrapper) + '\n')
print(wrapper)
