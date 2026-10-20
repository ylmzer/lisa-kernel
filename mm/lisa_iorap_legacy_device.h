/* SPDX-License-Identifier: GPL-2.0-only */
/* Bounded v2 transport; the separately gated PTE path matches this ROM's JNI. */
#ifdef CONFIG_LISA_IORAP_LEGACY
#ifndef LISA_IORAP_LEGACY_HOST_TEST
#include <linux/capability.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/user_namespace.h>
#endif
#include "lisa_iorap_legacy_request.h"
#include "lisa_iorap_legacy_wire.h"
#define LISA_IORAP_LEGACY_SLOTS 8
#define LISA_IORAP_LEGACY_CACHE_BYTES (16UL * 1024 * 1024)
struct lisa_iorap_legacy_cache {
	struct lisa_iorap_legacy_request key;
	unsigned char *blob;
	unsigned long bytes;
	unsigned long used;
};
static DEFINE_MUTEX(legacy_control);
static struct lisa_iorap_legacy_request legacy_active;
static struct lisa_iorap_legacy_cache legacy_cache[LISA_IORAP_LEGACY_SLOTS];
static unsigned long legacy_cache_bytes;
/* Private test-stage switch. A system daemon cannot enable itself before
 * root/CAP_SYS_ADMIN validation of the new driver has completed.
 */
#define LISA_IORAP_NATIVE_SWITCH 0x40044cfeU
static bool legacy_native_enabled;

static bool legacy_allowed(void)
{
	/* The actual ROM daemon is UID1000 without SYS_ADMIN. DAC and the
	 * existing lb_device SELinux type additionally gate every open/ioctl.
	 */
	if (current_user_ns() != &init_user_ns)
		return false;
	return uid_eq(current_euid(), GLOBAL_ROOT_UID) ?
		ns_capable(&init_user_ns, CAP_SYS_ADMIN) :
		__kuid_val(current_euid()) == 1000;
}
#include "lisa_iorap_rom_pte.h"
static int legacy_open(struct inode *inode, struct file *file)
{
	if (!legacy_allowed())
		return -EPERM;
	if (__kuid_val(current_euid()) == 1000 && !READ_ONCE(legacy_native_enabled))
		return -EAGAIN;
	return nonseekable_open(inode, file);
}
static void legacy_evict(unsigned int i)
{
	legacy_cache_bytes -= legacy_cache[i].bytes;
	kvfree(legacy_cache[i].blob);
	memset(&legacy_cache[i], 0, sizeof(legacy_cache[i]));
}
static int legacy_find(const struct lisa_iorap_legacy_request *key)
{
	unsigned int i;
	for (i = 0; i < LISA_IORAP_LEGACY_SLOTS; i++)
		if (legacy_cache[i].blob &&
		    lisa_iorap_legacy_same_app(key, &legacy_cache[i].key))
			return i;
	return -ENOENT;
}
static unsigned int legacy_oldest(void)
{
	unsigned int i, slot = LISA_IORAP_LEGACY_SLOTS;
	for (i = 0; i < LISA_IORAP_LEGACY_SLOTS; i++) {
		if (!legacy_cache[i].blob)
			continue;
		if (slot == LISA_IORAP_LEGACY_SLOTS ||
		    time_before(legacy_cache[i].used, legacy_cache[slot].used))
			slot = i;
	}
	return slot;
}
static unsigned int legacy_slot(void)
{
	unsigned int i;
	for (i = 0; i < LISA_IORAP_LEGACY_SLOTS; i++)
		if (!legacy_cache[i].blob)
			return i;
	return legacy_oldest();
}
static int legacy_finish(const struct lisa_iorap_legacy_request *key)
{
	struct lisa_iorap_snapshot *raw;
	struct lisa_iorap_bitmap_snapshot *snapshot;
	struct lisa_iorap_wire_view *views;
	unsigned char *blob = NULL;
	unsigned char prefix[LISA_IORAP_LEGACY_PREFIX] = { 0 };
	unsigned long i, bytes;
	int slot, error;
	if (!lisa_iorap_legacy_same_app(key, &legacy_active) ||
	    atomic_cmpxchg(&capture_transport_owner, 2, 3) != 2)
		return -ENOENT;
	raw = lisa_iorap_capture_end();
	if (IS_ERR(raw)) {
		error = PTR_ERR(raw);
		/* Allocation failure leaves the ring active: discard before releasing
		 * transport ownership, or a different endpoint could inherit it.
		 */
		lisa_iorap_capture_abort();
		goto owner;
	}
	snapshot = lisa_iorap_snapshot_bitmap(raw);
	lisa_iorap_snapshot_put(raw);
	if (IS_ERR(snapshot)) {
		error = PTR_ERR(snapshot);
		goto owner;
	}
	views = kcalloc(LISA_IORAP_MAX_FILES, sizeof(*views), GFP_KERNEL);
	if (!views) {
		error = -ENOMEM;
		goto snapshot;
	}
	for (i = 0; i < snapshot->files; i++) {
		struct lisa_iorap_file_bitmap *r = &snapshot->records[i];
		views[i].metadata[0] = r->mtime.tv_sec;
		views[i].metadata[1] = r->mtime.tv_nsec;
		views[i].metadata[2] = r->inode_number;
		views[i].metadata[3] = r->bits;
		views[i].metadata_verified = 1;
		views[i].path = (const unsigned char *)r->path;
		views[i].path_bytes = strlen(r->path);
		views[i].bitmap = r->bitmap;
		views[i].words = r->words;
		views[i].bits = r->bits;
	}
	if (lisa_iorap_wire_size(views, snapshot->files, &bytes)) {
		error = -EOVERFLOW;
		goto views;
	}
	if (bytes > LISA_IORAP_LEGACY_CACHE_BYTES) {
		error = -E2BIG;
		goto views;
	}
	blob = kvzalloc(bytes, GFP_KERNEL);
	if (!blob) {
		error = -ENOMEM;
		goto views;
	}
	lisa_iorap_wire_u32(prefix, key->uid);
	lisa_iorap_wire_u32(prefix + 4, key->pid);
	memcpy(prefix + 8, key->package, sizeof(key->package));
	if (lisa_iorap_wire_encode(blob, bytes, prefix, views, snapshot->files)) {
		error = -EINVAL;
		goto views;
	}
	slot = legacy_find(key);
	if (slot >= 0)
		legacy_evict(slot);
	while (legacy_cache_bytes > LISA_IORAP_LEGACY_CACHE_BYTES - bytes) {
		unsigned int victim = legacy_oldest();
		if (WARN_ON(victim >= LISA_IORAP_LEGACY_SLOTS)) {
			error = -EUCLEAN;
			goto views;
		}
		legacy_evict(victim);
	}
	if (slot < 0)
		slot = legacy_slot();
	legacy_evict(slot);
	legacy_cache[slot].key = *key;
	legacy_cache[slot].blob = blob;
	legacy_cache[slot].bytes = bytes;
	legacy_cache[slot].used = jiffies;
	legacy_cache_bytes += bytes;
	blob = NULL;
	error = 0;
views:
	kvfree(blob);
	kfree(views);
snapshot:
	lisa_iorap_bitmap_put(snapshot);
owner:
	atomic_cmpxchg(&capture_transport_owner, 3, 0);
	return error;
}
static long legacy_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct lisa_iorap_legacy_request key;
	enum lisa_iorap_legacy_operation op;
	unsigned char input[LISA_IORAP_LEGACY_INFO_HEADER];
	unsigned char *result = NULL;
	unsigned long capacity = 0, copy_bytes;
	__le64 size;
	void __user *user = (void __user *)arg;
	int slot, error = 0;
	(void)file;
	if (!legacy_allowed())
		return -EPERM;
	if (cmd == LISA_IORAP_NATIVE_SWITCH) {
		unsigned int enabled;
		if (!uid_eq(current_euid(), GLOBAL_ROOT_UID) ||
		    !ns_capable(&init_user_ns, CAP_SYS_ADMIN))
			return -EPERM;
		if (copy_from_user(&enabled, user, sizeof(enabled)))
			return -EFAULT;
		if (enabled > 1)
			return -EINVAL;
		mutex_lock(&legacy_control);
		WRITE_ONCE(legacy_native_enabled, enabled != 0);
		mutex_unlock(&legacy_control);
		return 0;
	}
	if (__kuid_val(current_euid()) == 1000 && !READ_ONCE(legacy_native_enabled))
		return -EAGAIN;
	op = lisa_iorap_legacy_operation(cmd);
	/* Names/debug/PTE are not silently emulated. The current real daemon
	 * only consumes size/info. Version ENOTTY selects its actual v2 path.
	 */
	if (!op || op == LISA_IORAP_LEGACY_NAMES)
		return -ENOTTY;
	copy_bytes = op == LISA_IORAP_LEGACY_INFO ?
		LISA_IORAP_LEGACY_INFO_HEADER : LISA_IORAP_LEGACY_PREFIX;
	if (copy_from_user(input, user, copy_bytes))
		return -EFAULT;
	if (lisa_iorap_legacy_decode(input, copy_bytes, &key))
		return -EINVAL;
	if (op == LISA_IORAP_LEGACY_INFO &&
	    lisa_iorap_legacy_capacity(input, copy_bytes, &capacity))
		return -EINVAL;
#ifdef CONFIG_LISA_IORAP_ROM_PTE
	if (op == LISA_IORAP_ROM_PTE_SNAPSHOT)
		return lisa_iorap_pte_snapshot(&key);
	if (op == LISA_IORAP_ROM_PTE_PREREAD)
		return lisa_iorap_pte_preread(&key);
	if (op == LISA_IORAP_ROM_PTE_STOP)
		return lisa_iorap_pte_stop(&key);
#endif
	mutex_lock(&legacy_control);
	if (atomic_read(&capture_transport_owner) == 1) {
		error = -EBUSY;
		goto out;
	}
	switch (op) {
	case LISA_IORAP_LEGACY_START:
		if (atomic_cmpxchg(&capture_transport_owner, 0, 2)) {
			error = -EBUSY;
			break;
		}
		legacy_active = key;
		error = lisa_iorap_capture_begin(make_kuid(&init_user_ns, key.uid));
		if (error)
			atomic_cmpxchg(&capture_transport_owner, 2, 0);
		break;
	case LISA_IORAP_LEGACY_FINISH:
		error = legacy_finish(&key);
		break;
	case LISA_IORAP_LEGACY_CLEAR:
	case LISA_IORAP_LEGACY_ABORT:
		if (lisa_iorap_legacy_same_app(&key, &legacy_active) &&
		    atomic_cmpxchg(&capture_transport_owner, 2, 3) == 2) {
			lisa_iorap_capture_abort();
			atomic_cmpxchg(&capture_transport_owner, 3, 0);
		} else if (op == LISA_IORAP_LEGACY_ABORT) {
			error = -ENOENT;
			break;
		}
		if (op == LISA_IORAP_LEGACY_CLEAR) {
			slot = legacy_find(&key);
			if (slot >= 0)
				legacy_evict(slot);
#ifdef CONFIG_LISA_IORAP_ROM_PTE
			lisa_iorap_pte_clear(&key);
#endif
		}
		break;
	case LISA_IORAP_LEGACY_SIZE:
		slot = legacy_find(&key);
		if (slot < 0) { error = slot; break; }
		size = cpu_to_le64(legacy_cache[slot].bytes);
		error = copy_to_user((unsigned char __user *)user + 264,
				     &size, sizeof(size)) ? -EFAULT : 0;
		legacy_cache[slot].used = jiffies;
		break;
	case LISA_IORAP_LEGACY_INFO:
		slot = legacy_find(&key);
		if (slot < 0) { error = slot; break; }
		if (capacity < legacy_cache[slot].bytes) {
			error = -ENOSPC;
			break;
		}
		/* Zero the entire caller-declared capacity before copying a result.
		 * The query may race a preceding collection, but undersized callers
		 * receive ENOSPC without any partial records or out-of-bounds write.
		 */
		result = kvzalloc(capacity, GFP_KERNEL);
		if (!result) { error = -ENOMEM; break; }
		memcpy(result, legacy_cache[slot].blob, legacy_cache[slot].bytes);
		memcpy(result, input, LISA_IORAP_LEGACY_PREFIX);
		lisa_iorap_wire_u64(result + 272, capacity);
		error = copy_to_user(user, result, capacity) ? -EFAULT : 0;
		legacy_cache[slot].used = jiffies;
		break;
	default:
		error = -ENOTTY;
	}
out:
	kvfree(result);
	mutex_unlock(&legacy_control);
	return error;
}
static const struct file_operations legacy_fops = {
	.owner = THIS_MODULE,
	.open = legacy_open,
	.unlocked_ioctl = legacy_ioctl,
	.llseek = no_llseek,
};
static struct miscdevice legacy_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "iorap_dev",
	.fops = &legacy_fops,
	.mode = 0600,
};
static int __init lisa_iorap_legacy_init(void)
{
	BUILD_BUG_ON(sizeof(struct lisa_iorap_legacy_request) != 264);
	return misc_register(&legacy_device);
}
device_initcall(lisa_iorap_legacy_init);
#endif
