/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _LINUX_LISA_IORAP_CAPTURE_H
#define _LINUX_LISA_IORAP_CAPTURE_H
#include <linux/types.h>
#include <linux/uidgid.h>

struct file;
struct lisa_iorap_snapshot;
struct lisa_iorap_bitmap_snapshot;

#ifdef CONFIG_LISA_IORAP_CAPTURE
void lisa_iorap_capture_page(struct file *file, pgoff_t index);
/* Process context only. Internal preparation API; no userspace device exists. */
int lisa_iorap_capture_begin(kuid_t uid);
struct lisa_iorap_snapshot *lisa_iorap_capture_end(void);
void lisa_iorap_snapshot_put(struct lisa_iorap_snapshot *snapshot);
/* Leaves the raw snapshot owned by caller, on both success and failure. */
struct lisa_iorap_bitmap_snapshot *lisa_iorap_snapshot_bitmap(
		const struct lisa_iorap_snapshot *snapshot);
void lisa_iorap_bitmap_put(struct lisa_iorap_bitmap_snapshot *snapshot);
#else
static inline void lisa_iorap_capture_page(struct file *file, pgoff_t index) {}
#endif
#endif
