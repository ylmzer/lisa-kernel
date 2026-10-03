"""Check upstream files that require a case-sensitive Linux checkout."""
import hashlib
import json
from pathlib import Path

root = Path(__file__).resolve().parents[2]
manifest = json.loads(Path(__file__).with_name('case-sensitive-source.json').read_text(encoding='utf-8'))
for relative, expected in manifest.items():
    data = (root / relative).read_bytes()
    actual = hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest()
    if actual != expected:
        raise SystemExit(f'Source mismatch: {relative}; use a fresh clone on a case-sensitive Linux filesystem.')
print(f'Verified {len(manifest)} case-sensitive upstream source files.')
