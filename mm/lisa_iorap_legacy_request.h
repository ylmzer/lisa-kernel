/* SPDX-License-Identifier: GPL-2.0-only */
/* Exact ARM64 v2 request decoding, shared by kernel transport and host tests.
 * This does not register a device or claim support for PTE/version ioctls.
 */
#ifndef LISA_IORAP_LEGACY_REQUEST_H
#define LISA_IORAP_LEGACY_REQUEST_H
#define LISA_IORAP_LEGACY_PREFIX 264UL
#define LISA_IORAP_LEGACY_INFO_HEADER 280UL
#define LISA_IORAP_LEGACY_LIMIT (10UL * 1024 * 1024)
struct lisa_iorap_legacy_request {
	unsigned int uid;
	unsigned int pid;
	unsigned char package[256];
};
enum lisa_iorap_legacy_operation {
	LISA_IORAP_LEGACY_UNSUPPORTED = 0,
	LISA_IORAP_LEGACY_START,
	LISA_IORAP_LEGACY_CLEAR,
	LISA_IORAP_LEGACY_FINISH,
	LISA_IORAP_LEGACY_ABORT,
	LISA_IORAP_LEGACY_SIZE,
	LISA_IORAP_LEGACY_INFO,
	LISA_IORAP_LEGACY_NAMES,
};
static inline unsigned int lisa_iorap_legacy_u32(const unsigned char *p)
{
	return (unsigned int)p[0] | (unsigned int)p[1] << 8 |
		(unsigned int)p[2] << 16 | (unsigned int)p[3] << 24;
}
static inline enum lisa_iorap_legacy_operation
lisa_iorap_legacy_operation(unsigned int cmd)
{
	switch (cmd) {
	case 0xc1084c01U: return LISA_IORAP_LEGACY_START;
	case 0xc1084c03U: return LISA_IORAP_LEGACY_CLEAR;
	case 0xc1084c05U: return LISA_IORAP_LEGACY_FINISH;
	case 0xc1084c06U: return LISA_IORAP_LEGACY_ABORT;
	case 0xc1084c07U: return LISA_IORAP_LEGACY_SIZE;
	case 0xc1084c08U: return LISA_IORAP_LEGACY_INFO;
	case 0xc1084c09U: return LISA_IORAP_LEGACY_NAMES;
	default: return LISA_IORAP_LEGACY_UNSUPPORTED;
	}
}
/* No unaligned loads or access beyond the independently copied byte count.
 * UID is first, PID second. JNI reporttokernel(pid, uid, name, action)
 * explicitly swaps its two Java integer parameters when constructing this.
 */
static inline int lisa_iorap_legacy_decode(
	const unsigned char *data, unsigned long bytes,
	struct lisa_iorap_legacy_request *out)
{
	unsigned long length, i;
	unsigned int uid, pid;
	if (!data || !out || bytes < LISA_IORAP_LEGACY_PREFIX)
		return -1;
	uid = lisa_iorap_legacy_u32(data);
	pid = lisa_iorap_legacy_u32(data + 4);
	if (uid > 0x7fffffffU || pid > 0x7fffffffU)
		return -1;
	for (length = 0; length < 256 && data[8 + length]; length++) {
		unsigned char c = data[8 + length];
		if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
		      (c >= '0' && c <= '9') || c == '.' || c == '_'))
			return -1;
	}
	if (!length || length == 256)
		return -1;
	/* Output remains untouched on every rejected request. */
	out->uid = uid;
	out->pid = pid;
	for (i = 0; i < 256; i++)
		out->package[i] = i < length ? data[8 + i] : 0;
	return 0;
}
static inline int lisa_iorap_legacy_capacity(
	const unsigned char *data, unsigned long bytes, unsigned long *capacity)
{
	unsigned long long value = 0;
	unsigned int i;
	if (!data || !capacity || bytes < LISA_IORAP_LEGACY_INFO_HEADER)
		return -1;
	for (i = 0; i < 8; i++)
		value |= (unsigned long long)data[272 + i] << (i * 8);
	if (value < LISA_IORAP_LEGACY_INFO_HEADER ||
	    value > LISA_IORAP_LEGACY_LIMIT)
		return -1;
	/* Input count/reserved are output-only and may be uninitialized in the
	 * real daemon's malloc buffer. They must not be interpreted as commands.
	 */
	*capacity = (unsigned long)value;
	return 0;
}
static inline int lisa_iorap_legacy_same_app(
	const struct lisa_iorap_legacy_request *a,
	const struct lisa_iorap_legacy_request *b)
{
	unsigned int i;
	if (!a || !b || a->uid != b->uid)
		return 0;
	/* PID is intentionally not a cache key: the native preread worker
	 * initializes it to zero while Java supplies the launched process PID.
	 */
	for (i = 0; i < 256; i++)
		if (a->package[i] != b->package[i])
			return 0;
	return 1;
}
#endif
