"""Fail closed: only explicitly allowlisted UTF-8 project sources may be published."""
import argparse
import json
import subprocess
from pathlib import Path

root = Path(__file__).resolve().parents[1]
allowed = set(json.loads((root / 'repository-files.json').read_text())['files'])
errors = []
def check(name, data):
    if name not in allowed:
        errors.append(f'Not allowlisted: {name}')
    if len(data) > 256 * 1024 or b'\0' in data:
        errors.append(f'Binary or oversized file: {name}')
    try:
        data.decode('utf-8-sig')
    except UnicodeDecodeError:
        errors.append(f'Not UTF-8 source: {name}')

parser = argparse.ArgumentParser()
parser.add_argument('--staged', action='store_true')
args = parser.parse_args()
for path in root.rglob('*'):
    name = path.relative_to(root).as_posix()
    if name == '.git' or name.startswith('.git/'):
        continue
    if path.is_symlink() or path.is_junction():
        errors.append(f'Linked path: {name}')
    elif path.is_file():
        check(name, path.read_bytes())
if args.staged:
    def git(*arguments):
        return subprocess.check_output(['git', '-C', str(root), *arguments])
    if not (root / '.git').exists():
        raise SystemExit('Initialize Git in this source directory before checking the index.')
    for entry in git('ls-files', '--stage', '-z').split(b'\0'):
        if not entry:
            continue
        metadata, name = entry.split(b'\t', 1)
        mode, blob, stage = metadata.split()
        if mode not in (b'100644', b'100755') or stage != b'0':
            errors.append(f'Unsupported index entry: {name!r}')
        check(name.decode(), git('cat-file', 'blob', blob.decode()))
if errors:
    raise SystemExit('\n'.join(errors))
print('PASS: only allowlisted UTF-8 project sources/configuration are present.')
