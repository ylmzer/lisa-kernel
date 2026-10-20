/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef LISA_IORAP_BITMAP_CORE_H
#define LISA_IORAP_BITMAP_CORE_H

#define LISA_IORAP_BITMAP_MAX_BITS (1UL << 19)

static inline int lisa_iorap_bitmap_words(unsigned long bits, unsigned long *words)
{
	if (!words || bits > LISA_IORAP_BITMAP_MAX_BITS)
		return -1;
	*words = (bits + 63) / 64;
	return 0;
}

static inline int lisa_iorap_bitmap_mark(unsigned long long *bitmap,
		unsigned long words, unsigned long bits, unsigned long index)
{
	unsigned long expected;
	unsigned long long mask;

	if (lisa_iorap_bitmap_words(bits, &expected) || words != expected ||
	    index >= bits || !bitmap)
		return -1;
	mask = 1ULL << (index & 63);
	if (bitmap[index / 64] & mask)
		return 0;
	bitmap[index / 64] |= mask;
	return 1;
}
#endif
