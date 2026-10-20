/* SPDX-License-Identifier: GPL-2.0-only */
/* Included by collector so raw file pointers never become an external API. */
#ifdef CONFIG_LISA_IORAP_TEST
#include <linux/capability.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/user_namespace.h>
#include <uapi/linux/lisa_iorap_test.h>

struct lisa_iorap_test_client {
	struct mutex lock;
	void *blob;
	size_t bytes;
};
static atomic_t test_opened = ATOMIC_INIT(0);

static bool test_allowed(void)
{
	return uid_eq(current_euid(), GLOBAL_ROOT_UID) &&
		ns_capable(&init_user_ns, CAP_SYS_ADMIN);
}

static int test_open(struct inode *inode, struct file *file)
{
	struct lisa_iorap_test_client *client;
	if (!test_allowed())
		return -EPERM;
	if (atomic_cmpxchg(&test_opened, 0, 1))
		return -EBUSY;
	client = kzalloc(sizeof(*client), GFP_KERNEL);
	if (!client) {
		atomic_set(&test_opened, 0);
		return -ENOMEM;
	}
	mutex_init(&client->lock);
	file->private_data = client;
	return nonseekable_open(inode, file);
}

static int test_stop(struct lisa_iorap_test_client *client)
{
	struct lisa_iorap_snapshot *raw;
	struct lisa_iorap_bitmap_snapshot *snapshot;
	struct lisa_iorap_test_header *header;
	struct lisa_iorap_test_record *record;
	struct lisa_iorap_file_bitmap *source;
	unsigned char *cursor;
	__le64 *bitmap;
	size_t bytes = sizeof(*header);
	unsigned long i, j;

	raw = lisa_iorap_capture_end();
	if (IS_ERR(raw))
		return PTR_ERR(raw);
	snapshot = lisa_iorap_snapshot_bitmap(raw);
	lisa_iorap_snapshot_put(raw);
	if (IS_ERR(snapshot))
		return PTR_ERR(snapshot);
	for (i = 0; i < snapshot->files; i++)
		bytes += sizeof(*record) + snapshot->records[i].words * 8;
	if (bytes > LISA_IORAP_TEST_MAX_BYTES) {
		lisa_iorap_bitmap_put(snapshot);
		return -E2BIG;
	}
	header = kvzalloc(bytes, GFP_KERNEL);
	if (!header) {
		lisa_iorap_bitmap_put(snapshot);
		return -ENOMEM;
	}
	header->magic = cpu_to_le32(LISA_IORAP_TEST_MAGIC);
	header->version = cpu_to_le32(1);
	header->uid = cpu_to_le32(snapshot->uid);
	header->files = cpu_to_le32(snapshot->files);
	header->bytes = cpu_to_le64(bytes);
	cursor = (unsigned char *)(header + 1);
	for (i = 0; i < snapshot->files; i++) {
		source = &snapshot->records[i];
		record = (void *)cursor;
		memcpy(record->path, source->path, sizeof(record->path));
		record->inode = cpu_to_le64(source->inode_number);
		record->device = cpu_to_le64(source->device);
		record->file_bytes = cpu_to_le64(source->file_bytes);
		record->mtime_sec = cpu_to_le64(source->mtime.tv_sec);
		record->mtime_nsec = cpu_to_le32(source->mtime.tv_nsec);
		record->bits = cpu_to_le32(source->bits);
		record->words = cpu_to_le32(source->words);
		bitmap = (void *)(record + 1);
		for (j = 0; j < source->words; j++)
			bitmap[j] = cpu_to_le64(source->bitmap[j]);
		cursor += sizeof(*record) + source->words * 8;
	}
	lisa_iorap_bitmap_put(snapshot);
	client->blob = header;
	client->bytes = bytes;
	return 0;
}

static long test_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct lisa_iorap_test_client *client = file->private_data;
	int result;
	if (!test_allowed())
		return -EPERM;
	if (arg)
		return -EINVAL;
	mutex_lock(&client->lock);
	switch (cmd) {
	case LISA_IORAP_TEST_START:
		result = client->blob ? -EBUSY : lisa_iorap_capture_begin(current_uid());
		break;
	case LISA_IORAP_TEST_STOP:
		result = client->blob ? -EBUSY : test_stop(client);
		file->f_pos = 0;
		break;
	case LISA_IORAP_TEST_ABORT:
		result = lisa_iorap_capture_abort();
		kvfree(client->blob);
		client->blob = NULL;
		client->bytes = 0;
		file->f_pos = 0;
		if (result == -ENOENT)
			result = 0; /* Discarding an already stopped client is idempotent. */
		break;
	default:
		result = -ENOTTY;
	}
	mutex_unlock(&client->lock);
	return result;
}

static ssize_t test_read(struct file *file, char __user *buf, size_t bytes,
			 loff_t *offset)
{
	struct lisa_iorap_test_client *client = file->private_data;
	ssize_t result;
	if (!test_allowed())
		return -EPERM;
	mutex_lock(&client->lock);
	result = client->blob ? simple_read_from_buffer(buf, bytes, offset,
		client->blob, client->bytes) : -ENODATA;
	mutex_unlock(&client->lock);
	return result;
}

static int test_release(struct inode *inode, struct file *file)
{
	struct lisa_iorap_test_client *client = file->private_data;
	/* VFS last close: no client operations remain. Cleanup must not depend
	 * on the closing task's credentials (which may have been dropped).
	 */
	mutex_lock(&capture_control);
	capture_discard_locked();
	cancel_delayed_work(&capture_timeout_work);
	mutex_unlock(&capture_control);
	kvfree(client->blob);
	kfree(client);
	atomic_set(&test_opened, 0);
	return 0;
}

static const struct file_operations test_fops = {
	.owner = THIS_MODULE,
	.open = test_open,
	.release = test_release,
	.unlocked_ioctl = test_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = test_ioctl,
#endif
	.read = test_read,
	.llseek = no_llseek,
};
static struct miscdevice test_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "lisa_iorap_test",
	.fops = &test_fops,
	.mode = 0600,
};
static int __init lisa_iorap_test_init(void)
{
	BUILD_BUG_ON(sizeof(struct lisa_iorap_test_header) != 24);
	BUILD_BUG_ON(sizeof(struct lisa_iorap_test_record) != 304);
	return misc_register(&test_device);
}
device_initcall(lisa_iorap_test_init);
#endif
