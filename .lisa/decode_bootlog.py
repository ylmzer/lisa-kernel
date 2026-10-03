"""Extract CRC-checked snapshots from a locally captured lisa oops partition."""
import argparse
import json
import struct
import zlib
from pathlib import Path

PART_BYTES = 16 * 1024 * 1024
SLOT_BYTES = 2 * 1024 * 1024
LOG_BYTES = 1024 * 1024
HEADER = struct.Struct('<8sIIIIQQQ64s400s')


def decode(data):
    if len(data) != PART_BYTES:
        raise ValueError('Expected a complete 16 MiB oops partition capture')
    records = []
    for slot in range(8):
        start = slot * SLOT_BYTES
        raw = data[start:start + HEADER.size]
        magic, version, length, crc, hcrc, generation, sequence, uptime, release, _ = HEADER.unpack(raw)
        if magic != b'LISABLG1' or version != 1 or not 0 < length <= LOG_BYTES:
            continue
        check = bytearray(raw)
        check[20:24] = b'\0' * 4
        if zlib.crc32(check) != hcrc:
            continue
        payload = data[start + HEADER.size:start + HEADER.size + length]
        if zlib.crc32(payload) != crc:
            continue
        if slot // 2 != generation % 4 or slot % 2 != sequence % 2:
            continue
        records.append({'slot': slot, 'generation': generation, 'sequence': sequence,
                        'uptime_seconds': uptime / 1e9,
                        'kernel': release.split(b'\0', 1)[0].decode(errors='replace'),
                        'payload': payload})
    return sorted(records, key=lambda x: (x['generation'], x['sequence']))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('partition', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    records = decode(args.partition.read_bytes())
    args.output.mkdir(parents=True, exist_ok=True)
    for record in records:
        name = f"boot-{record['generation']:04d}-snapshot-{record['sequence']:04d}.txt"
        (args.output / name).write_bytes(record.pop('payload'))
        record['file'] = name
    (args.output / 'index.json').write_text(json.dumps(records, indent=2) + '\n', encoding='utf-8')
    print(f'Extracted {len(records)} valid snapshots; unrecognized/corrupt records were ignored.')
