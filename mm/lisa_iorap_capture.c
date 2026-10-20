// SPDX-License-Identifier: GPL-2.0-only
/*
 * Experimental capture foundation, NOT a Xiaomi iorap ABI implementation.
 * No miscdevice, ioctl, proc command, or automatic start is registered.
 * Capture callbacks do not allocate, resolve paths, release files, or sleep.
 */
#include <linux/cred.h>
#include <linux/err.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/jump_label.h>
#include <linux/lisa_iorap_capture.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/vmalloc.h>
#include "lisa_iorap_capture_core.h"

#define LISA_IORAP_MAX_EVENTS 4096
#define LISA_IORAP_MAX_PAGES (1UL << 19) /* Limit a recorded file to 2 GiB at 4K. */

struct lisa_iorap_snapshot {
	unsigned int uid;
	unsigned long count;
	unsigned long dropped;
	struct lisa_iorap_event events[];
};

static DEFINE_MUTEX(capture_control);
static DEFINE_SPINLOCK(capture_lock);
static DEFINE_STATIC_KEY_FALSE(capture_enabled);
static struct lisa_iorap_event capture_events[LISA_IORAP_MAX_EVENTS];
static struct lisa_iorap_capture_state capture = {
	.events = capture_events,
	.capacity = LISA_IORAP_MAX_EVENTS,
	.max_pages = LISA_IORAP_MAX_PAGES,
};

void lisa_iorap_capture_page(struct file *file, pgoff_t index)
{
	unsigned long flags;
	int result;

	if (!static_branch_unlikely(&capture_enabled))
		return;
	if (!file || !S_ISREG(file_inode(file)->i_mode))
		return;
	spin_lock_irqsave(&capture_lock, flags);
	result = lisa_iorap_state_record(&capture, __kuid_val(current_uid()),
					file, index);
	/* The caller holds file alive; finish cannot transfer it before this ref. */
	if (result == LISA_IORAP_RECORDED)
		get_file(file);
	spin_unlock_irqrestore(&capture_lock, flags);
}

int lisa_iorap_capture_begin(kuid_t uid)
{
	unsigned long flags;
	int result;

	if (in_interrupt() || !uid_valid(uid))
		return -EINVAL;
	/* Future userspace control additionally needs domain-level SELinux checks. */
	if (!uid_eq(current_euid(), GLOBAL_ROOT_UID) &&
	    __kuid_val(current_euid()) != 1000)
		return -EPERM;
	mutex_lock(&capture_control);
	spin_lock_irqsave(&capture_lock, flags);
	result = lisa_iorap_state_begin(&capture, __kuid_val(uid));
	spin_unlock_irqrestore(&capture_lock, flags);
	if (!result)
		static_branch_enable(&capture_enabled);
	mutex_unlock(&capture_control);
	return result ? -EBUSY : 0;
}

struct lisa_iorap_snapshot *lisa_iorap_capture_end(void)
{
	struct lisa_iorap_snapshot *snapshot;
	unsigned long flags;

	if (in_interrupt())
		return ERR_PTR(-EINVAL);
	if (!uid_eq(current_euid(), GLOBAL_ROOT_UID) &&
	    __kuid_val(current_euid()) != 1000)
		return ERR_PTR(-EPERM);
	snapshot = kvzalloc(sizeof(*snapshot) + sizeof(capture_events), GFP_KERNEL);
	if (!snapshot)
		return ERR_PTR(-ENOMEM);
	mutex_lock(&capture_control);
	spin_lock_irqsave(&capture_lock, flags);
	if (lisa_iorap_state_end(&capture)) {
		spin_unlock_irqrestore(&capture_lock, flags);
		mutex_unlock(&capture_control);
		kvfree(snapshot);
		return ERR_PTR(-ENOENT);
	}
	spin_unlock_irqrestore(&capture_lock, flags);
	static_branch_disable(&capture_enabled);
	/* active is false under capture_lock; late callbacks can no longer append. */
	snapshot->uid = capture.uid;
	snapshot->count = capture.count;
	snapshot->dropped = capture.dropped;
	memcpy(snapshot->events, capture.events,
	       capture.count * sizeof(capture.events[0]));
	memset(capture.events, 0, sizeof(capture_events));
	capture.count = 0;
	mutex_unlock(&capture_control);
	return snapshot;
}

void lisa_iorap_snapshot_put(struct lisa_iorap_snapshot *snapshot)
{
	unsigned long i;

	if (IS_ERR_OR_NULL(snapshot))
		return;
	/* Never release transferred file references from an atomic context. */
	if (WARN_ON_ONCE(in_interrupt()))
		return;
	for (i = 0; i < snapshot->count; i++) {
		fput(snapshot->events[i].file);
		if (!(i & 255))
			cond_resched();
	}
	kvfree(snapshot);
}
