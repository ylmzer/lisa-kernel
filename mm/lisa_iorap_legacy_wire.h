/* SPDX-License-Identifier: GPL-2.0-only */
/* Observed legacy transport only. This file does not implement ioctl or guess
 * unknown metadata. The caller must supply independently verified metadata.
 */
#ifndef LISA_IORAP_LEGACY_WIRE_H
#define LISA_IORAP_LEGACY_WIRE_H
#include "lisa_iorap_bitmap_core.h"
#include "lisa_iorap_snapshot_budget.h"
#define LISA_IORAP_WIRE_HEADER 280UL
#define LISA_IORAP_WIRE_RECORD 296UL
#define LISA_IORAP_WIRE_MAX (10UL * 1024 * 1024)
#define LISA_IORAP_WIRE_MAX_RECORDS LISA_IORAP_SNAPSHOT_MAX_FILES

struct lisa_iorap_wire_view {
	unsigned long long metadata[4];
	const unsigned char *path;
	unsigned long path_bytes;
	const unsigned long long *bitmap;
	unsigned long words;
	unsigned long bits;
	int metadata_verified;
};

static inline void lisa_iorap_wire_u64(unsigned char *out, unsigned long long n)
{
	unsigned int i;
	for (i = 0; i < 8; i++)
		out[i] = (unsigned char)(n >> (i * 8));
}

static inline void lisa_iorap_wire_u32(unsigned char *out, unsigned int n)
{
	unsigned int i;
	for (i = 0; i < 4; i++)
		out[i] = (unsigned char)(n >> (i * 8));
}

/* Validation completes before encode writes even the first output byte. */
static inline int lisa_iorap_wire_size(const struct lisa_iorap_wire_view *records,
		unsigned long count, unsigned long *required)
{
	unsigned long i, j, words, n = LISA_IORAP_WIRE_HEADER, record_size;
	unsigned long bitmap_bytes = 0;
	const struct lisa_iorap_wire_view *r;

	if (!required || count > LISA_IORAP_WIRE_MAX_RECORDS || (count && !records))
		return -1;
	for (i = 0; i < count; i++) {
		r = &records[i];
		if (!r->metadata_verified)
			return -3; /* Metadata contract unresolved: never fabricate it. */
		if (!r->path || !r->path_bytes || r->path_bytes > 255 ||
		    r->path[0] != '/' ||
		    lisa_iorap_bitmap_words(r->bits, &words) || r->words != words ||
		    (words && !r->bitmap) ||
		    (unsigned int)r->metadata[3] != r->bits)
			return -1;
		for (j = 0; j < r->path_bytes; j++)
			if (!r->path[j])
				return -1;
		if (lisa_iorap_snapshot_reserve(i, &bitmap_bytes, words))
			return -2;
		record_size = LISA_IORAP_WIRE_RECORD + words * 8;
		if (record_size > LISA_IORAP_WIRE_MAX - n)
			return -2;
		n += record_size;
	}
	*required = n;
	return 0;
}

static inline int lisa_iorap_wire_encode(unsigned char *out, unsigned long capacity,
		const unsigned char *request_prefix,
		const struct lisa_iorap_wire_view *records, unsigned long count)
{
	unsigned long needed, i, j, offset;
	const struct lisa_iorap_wire_view *r;
	int error = lisa_iorap_wire_size(records, count, &needed);
	if (error)
		return error;
	if (!out || !request_prefix || capacity < needed ||
	    capacity > LISA_IORAP_WIRE_MAX)
		return -2;
	/* Callers must provide separate, immutable inputs (no output aliasing). */
	for (i = 0; i < capacity; i++)
		out[i] = 0;
	for (i = 0; i < 264; i++)
		out[i] = request_prefix[i];
	lisa_iorap_wire_u32(out + 264, count);
	lisa_iorap_wire_u64(out + 272, capacity);
	offset = LISA_IORAP_WIRE_HEADER;
	for (i = 0; i < count; i++) {
		r = &records[i];
		for (j = 0; j < 4; j++)
			lisa_iorap_wire_u64(out + offset + j * 8, r->metadata[j]);
		for (j = 0; j < r->path_bytes; j++)
			out[offset + 32 + j] = r->path[j];
		lisa_iorap_wire_u32(out + offset + 288, r->words);
		for (j = 0; j < r->words; j++)
			lisa_iorap_wire_u64(out + offset + LISA_IORAP_WIRE_RECORD + j * 8,
					  r->bitmap[j]);
		offset += LISA_IORAP_WIRE_RECORD + r->words * 8;
	}
	return 0;
}
#endif
