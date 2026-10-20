/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef _UAPI_LISA_IORAP_TEST_H
#define _UAPI_LISA_IORAP_TEST_H
#include <linux/types.h>
#include <linux/ioctl.h>
/* Dedicated diagnostic ABI. Never claims /dev/iorap_dev compatibility. */
#define LISA_IORAP_TEST_START _IO('Q', 1)
#define LISA_IORAP_TEST_STOP _IO('Q', 2)
#define LISA_IORAP_TEST_ABORT _IO('Q', 3)
#define LISA_IORAP_TEST_MAGIC 0x3154494cU
#define LISA_IORAP_TEST_MAX_BYTES (5U * 1024 * 1024)
struct lisa_iorap_test_header {
	__le32 magic, version, uid, files;
	__le64 bytes;
};
struct lisa_iorap_test_record {
	char path[256];
	__le64 inode, device, file_bytes, mtime_sec;
	__le32 mtime_nsec, bits, words, reserved;
	/* Followed by words little-endian u64 bitmap words. */
};
#endif
