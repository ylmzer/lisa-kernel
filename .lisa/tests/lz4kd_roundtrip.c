/* SPDX-License-Identifier: GPL-2.0-only */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "lz4kd.h"

int main(void)
{
	unsigned char *input = calloc(1, 8192);
	unsigned char *encoded = calloc(1, 16384);
	unsigned char *decoded = calloc(1, 16384);
	void *state = calloc(1, lz4kd_encode_state_bytes_min());
	unsigned completed = 0;
	uint32_t random = 0x6c697361;
	assert(input && encoded && decoded && state);
	for (unsigned page = 4096; page <= 8192; page *= 2) {
		for (unsigned pattern = 0; pattern < 5; ++pattern) {
			for (unsigned i = 0; i < page; ++i) {
				random ^= random << 13;
				random ^= random >> 17;
				random ^= random << 5;
				input[i] = pattern == 0 ? 0 : pattern == 1 ? 0xa5 :
					pattern == 2 ? i % 37 : pattern == 3 ?
					(i % 64 < 48 ? 0 : random) : random;
			}
			int compressed = lz4kd_encode(state, input, encoded, page, 16384, 0);
			assert(compressed >= 0);
			if (!compressed)
				continue; /* The API may reject incompressible pages. */
			assert(compressed <= 16384);
			memset(decoded, 0x5a, 16384);
			int length = lz4kd_decode(encoded, decoded, compressed, page);
			assert(length == (int)page);
			assert(memcmp(input, decoded, page) == 0);
			assert(lz4kd_decode(encoded, decoded, compressed, page / 2) < 0);
			assert(lz4kd_decode(encoded, decoded, 0, page) < 0);
			++completed;
		}
	}
	assert(completed >= 6);
	free(input);
	free(encoded);
	free(decoded);
	free(state);
	printf("LZ4KD: %u page round-trips passed; invalid lengths rejected.\n", completed);
	return 0;
}
