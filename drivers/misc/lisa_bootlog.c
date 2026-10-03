// SPDX-License-Identifier: GPL-2.0-only
/* Opt-in, bounded boot diagnosis for lisa's dedicated 16 MiB oops partition. */
#include <linux/blkdev.h>
#include <linux/bio.h>
#include <linux/crc32.h>
#include <linux/fs.h>
#include <linux/genhd.h>
#include <linux/init.h>
#include <linux/kmsg_dump.h>
#include <linux/ktime.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/reboot.h>
#include <linux/slab.h>
#include <linux/utsname.h>
#include <linux/vmalloc.h>
#include <linux/workqueue.h>

#define PART_BYTES (16UL * 1024 * 1024)
#define SLOT_BYTES (2UL * 1024 * 1024)
#define SLOT_COUNT 8
#define LOG_BYTES (1024UL * 1024)
#define HEADER_BYTES 4096
#define RUN_SECONDS 90

struct bootlog_header {
	u8 magic[8];
	__le32 version, length, data_crc, header_crc;
	__le64 generation, sequence, uptime_ns;
	char release[64];
	u8 reserved[400];
} __packed;

static bool enabled;
static bool stopped;
static struct block_device *logdev;
static char *logbuf;
static char *bounce;
static char *headerbuf;
static u64 generation, sequence;
static DEFINE_MUTEX(log_lock);
static struct delayed_work log_work;

static int __init bootlog_enable(char *value)
{
	enabled = value && !strcmp(value, "1");
	return 1;
}
__setup("lisa_bootlog=", bootlog_enable);

static u32 log_crc(const void *data, size_t len)
{
	return crc32_le(~0U, data, len) ^ ~0U;
}

static int transfer(void *data, size_t len, loff_t offset, bool write)
{
	char *bytes = data;

	if ((len | offset) & (HEADER_BYTES - 1))
		return -EINVAL;
	while (len) {
		size_t n = min_t(size_t, len, 64 * 1024), pos;
		struct bio *bio = bio_alloc(GFP_KERNEL, 16);
		int err;

		if (!bio)
			return -ENOMEM;
		if (write)
			memcpy(bounce, bytes, n);
		bio_set_dev(bio, logdev);
		bio->bi_iter.bi_sector = offset >> 9;
		bio->bi_opf = (write ? REQ_OP_WRITE : REQ_OP_READ) | REQ_SYNC;
		for (pos = 0; pos < n; pos += PAGE_SIZE) {
			unsigned int chunk = min_t(size_t, PAGE_SIZE, n - pos);

			if (bio_add_page(bio, virt_to_page(bounce + pos), chunk, 0) != chunk) {
				bio_put(bio);
				return -EIO;
			}
		}
		err = submit_bio_wait(bio);
		bio_put(bio);
		if (err)
			return err;
		if (!write)
			memcpy(bytes, bounce, n);
		bytes += n;
		len -= n;
		offset += n;
	}
	return 0;
}

static int open_log(void)
{
	dev_t dev;
	struct bootlog_header hdr;
	u64 latest = 0;
	int i;

	/* Do not depend on /dev nodes or userspace: wait for the GPT partition. */
	dev = blk_lookup_devt("sda", 17);
	if (!dev)
		return -EAGAIN;
	logdev = blkdev_get_by_dev(dev, FMODE_READ | FMODE_WRITE | FMODE_EXCL, &log_work);
	if (IS_ERR(logdev)) {
		int err = PTR_ERR(logdev);

		logdev = NULL;
		return err;
	}
	/* Refuse a whole disk, any other GPT label, or a different size. */
	if (!logdev->bd_part || !logdev->bd_part->info ||
	    strcmp((const char *)logdev->bd_part->info->volname, "oops") ||
	    i_size_read(logdev->bd_inode) != PART_BYTES ||
	    bdev_logical_block_size(logdev) > HEADER_BYTES) {
		blkdev_put(logdev, FMODE_READ | FMODE_WRITE | FMODE_EXCL);
		logdev = NULL;
		return -ENODEV;
	}
	for (i = 0; i < SLOT_COUNT; i++) {
		loff_t offset = (loff_t)i * SLOT_BYTES;
		u32 crc;

		if (transfer(headerbuf, HEADER_BYTES, offset, false))
			continue;
		memcpy(&hdr, headerbuf, sizeof(hdr));
		if (memcmp(hdr.magic, "LISABLG1", 8) ||
		    le32_to_cpu(hdr.version) != 1 ||
		    le32_to_cpu(hdr.length) > LOG_BYTES)
			continue;
		crc = le32_to_cpu(hdr.header_crc);
		hdr.header_crc = 0;
		if (crc == log_crc(&hdr, sizeof(hdr)))
			latest = max(latest, le64_to_cpu(hdr.generation));
	}
	if (latest == U64_MAX) {
		blkdev_put(logdev, FMODE_READ | FMODE_WRITE | FMODE_EXCL);
		logdev = NULL;
		return -EOVERFLOW;
	}
	generation = latest + 1;
	pr_info("lisa_bootlog: recording generation %llu to verified oops partition\n",
		(unsigned long long)generation);
	return 0;
}

/* Called only in sleepable work/notifier context, never from a panic hook. */
static int snapshot(void)
{
	struct kmsg_dumper reader = { .active = true };
	struct bootlog_header hdr = { };
	size_t len = 0;
	loff_t offset;
	int err;

	kmsg_dump_rewind(&reader);
	if (!kmsg_dump_get_buffer(&reader, true, logbuf, LOG_BYTES, &len))
		return 0;
	memcpy(hdr.magic, "LISABLG1", 8);
	hdr.version = cpu_to_le32(1);
	hdr.length = cpu_to_le32(len);
	hdr.data_crc = cpu_to_le32(log_crc(logbuf, len));
	hdr.generation = cpu_to_le64(generation);
	hdr.sequence = cpu_to_le64(++sequence);
	hdr.uptime_ns = cpu_to_le64(ktime_get_boottime_ns());
	strlcpy(hdr.release, init_uts_ns.name.release, sizeof(hdr.release));
	hdr.header_crc = cpu_to_le32(log_crc(&hdr, sizeof(hdr)));
	/* Two alternating snapshots per boot, with four boots retained. */
	offset = ((generation % 4) * 2 + (sequence % 2)) * SLOT_BYTES;
	memset(logbuf + len, 0, ALIGN(len, HEADER_BYTES) - len);
	err = transfer(logbuf, ALIGN(len, HEADER_BYTES), offset + HEADER_BYTES, true);
	memset(headerbuf, 0, HEADER_BYTES);
	memcpy(headerbuf, &hdr, sizeof(hdr));
	if (!err)
		err = transfer(headerbuf, HEADER_BYTES, offset, true);
	if (!err)
		err = blkdev_issue_flush(logdev, GFP_KERNEL, NULL);
	return err;
}

static void collect(struct work_struct *work)
{
	int err;
	u64 seconds = ktime_get_boottime_seconds();

	mutex_lock(&log_lock);
	if (stopped || seconds >= RUN_SECONDS)
		goto stop;
	if (!logdev) {
		err = open_log();
		if (err) {
			if (err == -ENODEV || err == -EOVERFLOW) {
				pr_err("lisa_bootlog: partition validation failed (%d), no write\n", err);
				goto stop;
			}
			goto again;
		}
	}
	err = snapshot();
	if (err) {
		pr_err("lisa_bootlog: I/O error %d; recording stopped\n", err);
		goto stop;
	}
again:
	mutex_unlock(&log_lock);
	schedule_delayed_work(&log_work, msecs_to_jiffies(seconds < 30 ? 500 : 2000));
	return;
stop:
	stopped = true;
	if (logdev) {
		blkdev_put(logdev, FMODE_READ | FMODE_WRITE | FMODE_EXCL);
		logdev = NULL;
	}
	mutex_unlock(&log_lock);
}

static int before_reboot(struct notifier_block *nb, unsigned long action, void *data)
{
	mutex_lock(&log_lock);
	if (!stopped && logdev)
		snapshot();
	stopped = true;
	mutex_unlock(&log_lock);
	cancel_delayed_work_sync(&log_work);
	return NOTIFY_DONE;
}

static struct notifier_block reboot_nb = { .notifier_call = before_reboot };

static int __init lisa_bootlog_init(void)
{
	const char *model;

	BUILD_BUG_ON(sizeof(struct bootlog_header) != 512);
	BUILD_BUG_ON(PAGE_SIZE != HEADER_BYTES);
	if (!enabled)
		return 0;
	if (of_property_read_string(of_root, "model", &model) ||
	    strcmp(model, "lisa based Qualcomm Technologies, Inc. SM7325")) {
		pr_err("lisa_bootlog: unexpected board, disabled\n");
		return 0;
	}
	logbuf = vzalloc(LOG_BYTES);
	bounce = kmalloc(64 * 1024, GFP_KERNEL);
	headerbuf = kmalloc(HEADER_BYTES, GFP_KERNEL);
	if (!logbuf || !bounce || !headerbuf) {
		vfree(logbuf);
		kfree(bounce);
		kfree(headerbuf);
		return -ENOMEM;
	}
	INIT_DELAYED_WORK(&log_work, collect);
	if (register_reboot_notifier(&reboot_nb)) {
		vfree(logbuf);
		kfree(bounce);
		kfree(headerbuf);
		return -EINVAL;
	}
	pr_info("lisa_bootlog: opt-in recorder armed for the first %d seconds\n", RUN_SECONDS);
	schedule_delayed_work(&log_work, 0);
	return 0;
}
late_initcall(lisa_bootlog_init);
