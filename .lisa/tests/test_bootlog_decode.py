import importlib.util
import struct
import unittest
import zlib
from pathlib import Path

spec = importlib.util.spec_from_file_location('bootlog', Path(__file__).parents[1] / 'decode_bootlog.py')
bootlog = importlib.util.module_from_spec(spec)
spec.loader.exec_module(bootlog)


def put(data, generation, sequence, payload):
    slot = generation % 4 * 2 + sequence % 2
    start = slot * bootlog.SLOT_BYTES
    header = bytearray(bootlog.HEADER.pack(b'LISABLG1', 1, len(payload), zlib.crc32(payload),
                                         0, generation, sequence, 2000000000,
                                         b'5.4.302-lisa-bootdiag', b''))
    struct.pack_into('<I', header, 20, zlib.crc32(header))
    data[start:start + 512] = header
    data[start + bootlog.HEADER_BYTES:start + bootlog.HEADER_BYTES + len(payload)] = payload
    return start


class BootlogDecode(unittest.TestCase):
    def test_torn_latest_write_retains_previous_snapshot(self):
        data = bytearray(bootlog.PART_BYTES)
        put(data, 1, 1, b'init: previous\n')
        start = put(data, 1, 2, b'init: latest\n')
        data[start + bootlog.HEADER_BYTES] ^= 1
        records = bootlog.decode(data)
        self.assertEqual([(r['generation'], r['sequence']) for r in records], [(1, 1)])

    def test_recovery_boot_preserves_failed_boot_records(self):
        data = bytearray(bootlog.PART_BYTES)
        for gen in range(1, 6):
            put(data, gen, 1, f'kernel generation {gen}\n'.encode())
        self.assertEqual([r['generation'] for r in bootlog.decode(data)], [2, 3, 4, 5])

    def test_corrupt_header_and_partial_capture_are_rejected(self):
        data = bytearray(bootlog.PART_BYTES)
        start = put(data, 1, 1, b'init: message\n')
        data[start + 24] ^= 1
        self.assertEqual(bootlog.decode(data), [])
        with self.assertRaises(ValueError):
            bootlog.decode(data[:-1])


if __name__ == '__main__':
    unittest.main()
