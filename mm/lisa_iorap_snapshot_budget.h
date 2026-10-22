/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef LISA_IORAP_SNAPSHOT_BUDGET_H
#define LISA_IORAP_SNAPSHOT_BUDGET_H
/* Counts are dynamic in the observed v2 consumer. Keep both dimensions bounded:
 * 1024 record headers plus 8 MiB bitmaps fit the existing 10 MiB wire limit.
 * These are resource limits, not claims about the device workload's file count.
 */
#define LISA_IORAP_SNAPSHOT_MAX_FILES 1024UL
#define LISA_IORAP_SNAPSHOT_BITMAP_BYTES (8UL * 1024 * 1024)
static inline int lisa_iorap_snapshot_reserve(unsigned long files,
        unsigned long *bitmap_bytes, unsigned long words)
{
    unsigned long bytes;
    if (!bitmap_bytes || files >= LISA_IORAP_SNAPSHOT_MAX_FILES ||
        *bitmap_bytes > LISA_IORAP_SNAPSHOT_BITMAP_BYTES ||
        words > (LISA_IORAP_SNAPSHOT_BITMAP_BYTES - *bitmap_bytes) / 8)
        return -1;
    bytes = words * 8;
    *bitmap_bytes += bytes;
    return 0;
}
#endif
