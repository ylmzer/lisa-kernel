"""Read a connected lisa's oops log partition without changing the phone."""
import argparse
import datetime
import hashlib
import json
import os
import subprocess
from pathlib import Path

from decode_bootlog import PART_BYTES, decode

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--adb', default=str(Path(os.environ.get('LOCALAPPDATA', '')) / 'Android/Sdk/platform-tools/adb.exe'))
parser.add_argument('--serial', default='6d202a13')
args = parser.parse_args()
prefix = [args.adb, '-s', args.serial]


def run(*command):
    result = subprocess.run(prefix + list(command), capture_output=True, timeout=30)
    if result.returncode:
        raise RuntimeError(f'ADB read failed: {command[0]}: {result.stderr.decode(errors="replace")}')
    return result.stdout


device = run('shell', 'getprop', 'ro.product.device').decode().strip()
if device != 'lisa':
    raise SystemExit('Expected an authorized lisa device; no partition was read')
size = int(run('shell', 'blockdev', '--getsize64', '/dev/block/by-name/oops').strip())
if size != PART_BYTES:
    raise SystemExit('Unexpected oops partition size; no partition was read')
stamp = datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=3))).strftime('%Y%m%d-%H%M%S')
root = Path(__file__).resolve().parent / 'private/captures' / (stamp + '-bootdiag')
root.mkdir(parents=True)
identity = run('shell', 'uname -a; cat /proc/cmdline; cat /proc/sys/kernel/random/boot_id')
(root / 'identity.txt').write_bytes(identity)
data = run('exec-out', 'dd', 'if=/dev/block/by-name/oops', 'bs=1048576', 'status=none')
if len(data) != PART_BYTES:
    raise SystemExit('Incomplete oops partition read; no valid capture saved')
(root / 'oops-partition.bin').write_bytes(data)
records = decode(data)
for record in records:
    name = f"boot-{record['generation']:04d}-snapshot-{record['sequence']:04d}.txt"
    (root / name).write_bytes(record.pop('payload'))
    record['file'] = name
(root / 'index.json').write_text(json.dumps({'partition_sha256': hashlib.sha256(data).hexdigest(),
                                          'snapshots': records}, indent=2) + '\n', encoding='utf-8')
print(f'Read-only capture saved: {root}')
print(f'Valid snapshots: {len(records)}')
if not records:
    print('No diagnostic records found; recording has not been demonstrated.')
