// SPDX-License-Identifier: GPL-2.0-only
/*
 * Bounded capture foundation with separately gated diagnostic/ROM transports.
 * All features default off; callbacks never start a session automatically.
 * Capture callbacks do not allocate, resolve paths, release files, or sleep.
 */
#include <linux/cred.h>
#include <linux/dcache.h>
#include <linux/err.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/ratelimit.h>
#include <linux/jump_label.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/lisa_iorap_capture.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include "lisa_iorap_capture_core.h"
#include "lisa_iorap_bitmap_core.h"
#include "lisa_iorap_snapshot_budget.h"

#define LISA_IORAP_MAX_EVENTS 65536 /* Measured cold launch: 51988 callbacks. */
#define LISA_IORAP_MAX_PAGES (1UL << 19) /* Limit a recorded file to 2 GiB at 4K. */
#define LISA_IORAP_MAX_FILES LISA_IORAP_SNAPSHOT_MAX_FILES

struct lisa_iorap_file_bitmap {
	unsigned long long inode_number;
	unsigned long long device;
	loff_t file_bytes;
	struct timespec64 mtime;
	struct timespec64 ctime;
	unsigned long bits;
	unsigned long words;
	unsigned long unique_pages;
	unsigned long long *bitmap;
	char path[256];
};

struct lisa_iorap_bitmap_snapshot {
	unsigned int uid;
	unsigned long files;
	struct lisa_iorap_file_bitmap records[];
};

struct lisa_iorap_snapshot {
	unsigned int uid;
	unsigned long count;
	unsigned long dropped;
	struct lisa_iorap_event events[];
};

static DEFINE_MUTEX(capture_control);
/* 0 idle, 1 diagnostic fd, 2 legacy collection, 3 legacy finish/abort.
 * Timeout can release 2 but must never release 3 while its owner is working.
 */
static atomic_t capture_transport_owner = ATOMIC_INIT(0);
static DEFINE_SPINLOCK(capture_lock);
static DEFINE_STATIC_KEY_FALSE(capture_enabled);
static unsigned long capture_deadline;
static void capture_timeout(struct work_struct *work);
static DECLARE_DELAYED_WORK(capture_timeout_work, capture_timeout);
static struct lisa_iorap_event capture_events[LISA_IORAP_MAX_EVENTS];
static unsigned int capture_seen[LISA_IORAP_MAX_EVENTS * 2];
static struct lisa_iorap_capture_state capture = {
	.events = capture_events,
	.seen = capture_seen,
	.seen_capacity = ARRAY_SIZE(capture_seen),
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
	result = lisa_iorap_state_record_mapping(&capture, __kuid_val(current_uid()),
					file, file->f_mapping, index);
	/* The caller holds file alive; finish cannot transfer it before this ref. */
	if (result == LISA_IORAP_RECORDED)
		get_file(file);
	spin_unlock_irqrestore(&capture_lock, flags);
}

/* capture_control held; no allocation is required to abandon an active ring. */
static int capture_discard_locked(void)
{
	unsigned long flags, i;
	spin_lock_irqsave(&capture_lock, flags);
	if (lisa_iorap_state_end(&capture)) {
		spin_unlock_irqrestore(&capture_lock, flags);
		return -ENOENT;
	}
	spin_unlock_irqrestore(&capture_lock, flags);
	static_branch_disable(&capture_enabled);
	for (i = 0; i < capture.count; i++) {
		fput(capture.events[i].file);
		capture.events[i].file = NULL;
		if (!(i & 255))
			cond_resched();
	}
	capture.count = 0;
	return 0;
}

static void capture_timeout(struct work_struct *work)
{
	unsigned long now;
	mutex_lock(&capture_control);
	now = jiffies;
	/* A queued callback from an earlier session must not stop a newer one. */
	if (capture.active) {
		if (time_before(now, capture_deadline))
			mod_delayed_work(system_wq, &capture_timeout_work,
					 capture_deadline - now);
		else if (!capture_discard_locked())
			atomic_cmpxchg(&capture_transport_owner, 2, 0);
	}
	mutex_unlock(&capture_control);
}

int lisa_iorap_capture_abort(void)
{
	int result;
	if (in_interrupt())
		return -EINVAL;
	if (!uid_eq(current_euid(), GLOBAL_ROOT_UID) &&
	    __kuid_val(current_euid()) != 1000)
		return -EPERM;
	mutex_lock(&capture_control);
	result = capture_discard_locked();
	cancel_delayed_work(&capture_timeout_work);
	mutex_unlock(&capture_control);
	return result;
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
	if (!result) {
		static_branch_enable(&capture_enabled);
		capture_deadline = jiffies + msecs_to_jiffies(30000);
		mod_delayed_work(system_wq, &capture_timeout_work,
				 msecs_to_jiffies(30000));
	}
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
	cancel_delayed_work(&capture_timeout_work);
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

void lisa_iorap_bitmap_put(struct lisa_iorap_bitmap_snapshot *snapshot)
{
	unsigned long i;

	if (IS_ERR_OR_NULL(snapshot))
		return;
	if (WARN_ON_ONCE(in_interrupt()))
		return;
	for (i = 0; i < snapshot->files; i++)
		kvfree(snapshot->records[i].bitmap);
	kvfree(snapshot);
}

/* Process-context consolidation. No unverified Xiaomi metadata is emitted. */
struct lisa_iorap_bitmap_snapshot *lisa_iorap_snapshot_bitmap(
		const struct lisa_iorap_snapshot *snapshot)
{
	struct lisa_iorap_bitmap_snapshot *result;
	struct file **keys = NULL;
	struct lisa_iorap_file_bitmap *r;
	struct inode *inode;
	struct file *file;
	unsigned long i, j, words, bitmap_bytes = 0;
	char *path;
	int error = 0, marked;

	if (in_interrupt() || IS_ERR_OR_NULL(snapshot))
		return ERR_PTR(-EINVAL);
	if (!uid_eq(current_euid(), GLOBAL_ROOT_UID) &&
	    __kuid_val(current_euid()) != 1000)
		return ERR_PTR(-EPERM);
	/* A truncated capture must not become a seemingly complete bitmap. */
	if (snapshot->dropped || snapshot->count > LISA_IORAP_MAX_EVENTS) {
		pr_warn_ratelimited("lisa_iorap: incomplete capture count=%lu dropped=%lu\n",
				    snapshot->count, snapshot->dropped);
		return ERR_PTR(-EOVERFLOW);
	}
	result = kvzalloc(sizeof(*result) + LISA_IORAP_MAX_FILES * sizeof(*r),
			 GFP_KERNEL);
	if (!result)
		return ERR_PTR(-ENOMEM);
	keys = kvcalloc(LISA_IORAP_MAX_FILES, sizeof(*keys), GFP_KERNEL);
	if (!keys) {
		error = -ENOMEM;
		goto fail;
	}
	result->uid = snapshot->uid;
	for (i = 0; i < snapshot->count; i++) {
		file = snapshot->events[i].file;
		inode = file_inode(file);
		for (j = 0; j < result->files; j++)
			if (file_inode(keys[j]) == inode &&
			    keys[j]->f_path.mnt == file->f_path.mnt)
				break;
		if (j == result->files) {
			if (j == LISA_IORAP_MAX_FILES) {
				pr_warn_ratelimited("lisa_iorap: file quota=%lu events=%lu\n",
						    result->files, snapshot->count);
				error = -E2BIG;
				goto fail;
			}
			r = &result->records[j];
			inode_lock(inode);
			r->file_bytes = i_size_read(inode);
			r->mtime = inode->i_mtime;
			r->ctime = inode->i_ctime;
			r->inode_number = inode->i_ino;
			r->device = inode->i_sb->s_dev;
			inode_unlock(inode);
			if (r->file_bytes <= 0 ||
			    r->file_bytes > (loff_t)LISA_IORAP_MAX_PAGES * PAGE_SIZE) {
				error = -ERANGE;
				goto fail;
			}
			r->bits = DIV_ROUND_UP_ULL(r->file_bytes, PAGE_SIZE);
			if (lisa_iorap_bitmap_words(r->bits, &words)) {
				error = -ERANGE;
				goto fail;
			}
			if (lisa_iorap_snapshot_reserve(result->files, &bitmap_bytes, words)) {
				pr_warn_ratelimited("lisa_iorap: bitmap budget files=%lu bytes=%lu words=%lu\n",
						    result->files, bitmap_bytes, words);
				error = -E2BIG;
				goto fail;
			}
			r->words = words;
			path = file_path(file, r->path, sizeof(r->path));
			if (IS_ERR(path)) {
				error = PTR_ERR(path);
				goto fail;
			}
			if (path[0] != '/' || d_unlinked(file->f_path.dentry)) {
				error = -ESTALE;
				goto fail;
			}
			memmove(r->path, path, strlen(path) + 1);
			r->bitmap = kvcalloc(words, sizeof(*r->bitmap), GFP_KERNEL);
			if (!r->bitmap) {
				error = -ENOMEM;
				goto fail;
			}
			keys[j] = file;
			/* Cleanup owns the bitmap from this point, even if marking fails. */
			result->files++;
		}
		r = &result->records[j];
		marked = lisa_iorap_bitmap_mark(r->bitmap, r->words, r->bits,
					      snapshot->events[i].index);
		if (marked < 0) {
			error = -ESTALE;
			goto fail;
		}
		r->unique_pages += marked;
		if (!(i & 255))
			cond_resched();
	}
	/* Detect truncation or modification during consolidation. Pinned files in
	 * the raw snapshot keep every inode alive; keys never escape this function.
	 */
	for (j = 0; j < result->files; j++) {
		inode = file_inode(keys[j]);
		r = &result->records[j];
		inode_lock(inode);
		if (r->file_bytes != i_size_read(inode) ||
		    !timespec64_equal(&r->mtime, &inode->i_mtime) ||
		    !timespec64_equal(&r->ctime, &inode->i_ctime))
			error = -ESTALE;
		inode_unlock(inode);
		if (error || d_unlinked(keys[j]->f_path.dentry)) {
			error = -ESTALE;
			goto fail;
		}
	}
	pr_info_ratelimited("lisa_iorap: snapshot files=%lu events=%lu bitmap_bytes=%lu\n",
			    result->files, snapshot->count, bitmap_bytes);
	kvfree(keys);
	return result;
fail:
	kvfree(keys);
	lisa_iorap_bitmap_put(result);
	return ERR_PTR(error);
}

#include "lisa_iorap_test_device.h"
#include "lisa_iorap_legacy_device.h"
