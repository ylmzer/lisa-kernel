/* SPDX-License-Identifier: GPL-2.0-only */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../mm/lisa_iorap_legacy_wire.h"
int main(int argc, char **argv)
{
 unsigned long words, size, i;
 unsigned long long map[3] = {0};
 unsigned char prefix[264] = {0}, out[1024], before[1024], path[256];
 struct lisa_iorap_wire_view r = {{123, 456, 789, 130},
  (const unsigned char *)"/data/test", 10, map, 3, 130, 1};
 FILE *f;
 assert(!lisa_iorap_bitmap_words(130, &words) && words == 3);
 assert(lisa_iorap_bitmap_words(LISA_IORAP_BITMAP_MAX_BITS+1, &words) == -1);
 assert(lisa_iorap_bitmap_mark(map, 3, 130, 0) == 1);
 assert(lisa_iorap_bitmap_mark(map, 3, 130, 63) == 1);
 assert(lisa_iorap_bitmap_mark(map, 3, 130, 64) == 1);
 assert(lisa_iorap_bitmap_mark(map, 3, 130, 129) == 1);
 assert(lisa_iorap_bitmap_mark(map, 3, 130, 64) == 0);
 assert(lisa_iorap_bitmap_mark(map, 3, 130, 130) == -1);
 assert(map[0] == (1ULL | (1ULL<<63)) && map[1] == 1 && map[2] == 2);
 assert(!lisa_iorap_wire_size(&r, 1, &size) && size == 600);
 memset(out, 0xa5, sizeof(out)); memcpy(before, out, sizeof(out));
 r.metadata_verified = 0;
 assert(lisa_iorap_wire_encode(out, sizeof(out), prefix, &r, 1) == -3);
 assert(!memcmp(out, before, sizeof(out))); r.metadata_verified = 1;
 assert(lisa_iorap_wire_encode(out, size-1, prefix, &r, 1) == -2);
 assert(!memcmp(out, before, sizeof(out)));
 r.metadata[3] = 129;
 assert(lisa_iorap_wire_encode(out, sizeof(out), prefix, &r, 1) == -1);
 assert(!memcmp(out, before, sizeof(out))); r.metadata[3] = 130;
 memset(path, 'a', sizeof(path)); path[0] = '/'; r.path = path;
 r.path_bytes = 256; assert(lisa_iorap_wire_size(&r, 1, &size) == -1);
 r.path_bytes = 255; assert(!lisa_iorap_wire_size(&r, 1, &size));
 path[5] = 0; assert(lisa_iorap_wire_size(&r, 1, &size) == -1);
 r.path = (const unsigned char *)"/data/test"; r.path_bytes = 10;
 assert(!lisa_iorap_wire_encode(out, 600, prefix, &r, 1));
 assert(out[264] == 1 && out[272] == 88 && out[273] == 2);
 assert(out[280] == 123 && out[568] == 3 && out[583] == 128);
 for (i = 600; i < sizeof(out); i++) assert(out[i] == 0xa5);
 if (argc == 2) {
  f = fopen(argv[1], "wb"); assert(f);
  assert(fwrite(out, 1, 600, f) == 600); assert(!fclose(f));
 }
 assert(!lisa_iorap_wire_encode(out, 280, prefix, NULL, 0));
 assert(out[264] == 0);
 puts("PASS bitmap boundaries, deduplication, wire preflight and synthetic response");
 return 0;
}
