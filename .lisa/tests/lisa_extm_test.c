/* SPDX-License-Identifier: GPL-2.0 */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
/* Current zram_table_entry flags end at bit 30; exercise the real bit layout. */
#define LISA_EXTM_AGE_SHIFT 30
#include "../../drivers/block/zram/lisa_extm.h"
static void parse_ok(const char *s, unsigned long max, unsigned int age, bool idle)
{
 struct lisa_extm_request req = { 0, 0, false };
 assert(lisa_extm_parse(s, strlen(s), &req));
 assert(req.max_pages == max && req.min_age == age && req.idle == idle);
}
int main(void)
{
 struct lisa_extm_request req = { 42, 7, false };
 const char *bad[] = { "", "id", "idleX", "idle -1", "idle +1", "idle 1 -1",
  "idle 1 2 3", "huge 1", "idle 18446744073709551616", "idle 1x", "idle 1 2x" };
 unsigned long flags = (1UL << 63) | ((1UL << LISA_EXTM_AGE_SHIFT) - 1);
 unsigned long original = flags;
 unsigned int i;
 assert(sizeof(unsigned long) * CHAR_BIT >= LISA_EXTM_AGE_SHIFT + 4);
 for (i = 0; i < 100; i++) {
  flags = lisa_extm_mark(flags);
  assert(lisa_extm_age(flags) == (i < 10 ? i + 1 : 10));
  assert(lisa_extm_clear_age(flags) == original);
 }
 assert(lisa_extm_age(lisa_extm_clear_age(flags)) == 0);
 parse_ok("idle", ULONG_MAX, 1, true);
 parse_ok("idle\n", ULONG_MAX, 1, true);
 parse_ok("huge\n", ULONG_MAX, 1, false);
 parse_ok("idle 0 0", 0, 0, true);
 parse_ok("idle 2000 5\n", 2000, 5, true);
 parse_ok("idle 10 99", 10, 10, true);
 parse_ok("idle 18446744073709551615 10", ULONG_MAX, 10, true);
 for (i = 0; i < sizeof(bad) / sizeof(*bad); i++) {
  assert(!lisa_extm_parse(bad[i], strlen(bad[i]), &req));
  assert(req.max_pages == 42 && req.min_age == 7 && !req.idle);
 }
 assert(!lisa_extm_parse("idle\0x", 6, &req));
 assert(!lisa_extm_parse("idle", 129, &req));
 puts("ExtM age saturation, flag preservation, reset and bounded request parsing passed");
 return 0;
}
