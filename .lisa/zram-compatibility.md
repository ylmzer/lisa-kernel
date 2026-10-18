# Lisa HyperOS ZRAM and storage writeback

Scope: memory compatibility only; no display, lockscreen, launcher or panel settings.

## Verified current device, 18 October 2026
The installed main kernel has CONFIG_ZRAM, ZSMALLOC, SWAP and LZ4KD. Android reports 8,388,604 KiB total swap. One sample reports 49,596 KiB swapped data using 13,112 KiB physical ZRAM memory. This demonstrates active compressed swap, not a stress/stability qualification. Direct adb shell access to ZRAM sysfs and /proc/swaps is denied under enforcing; this is not evidence that ZRAM itself failed.

The installed kernel has ZRAM_WRITEBACK disabled. The build configuration now enables the existing standard implementation, preserving the raw reference config. zram-only CI compiles the actual driver and zsmalloc and checks writeback provider symbols. Merely building the feature does not allocate storage or activate writeback.

## ROM interfaces and remaining work
ROM init configures idle/writeback/writeback_limit controls. It also mentions Xiaomi-specific new, mfz_*, zgroup_enable, qpace_enable and memory.zram_writeback_control/anno_writeback_* interfaces, which this source does not implement. Standard writeback must not be advertised as implementing these OEM features. Do not add placeholder sysfs files or successful no-op handlers.

/system_ext/etc/init/memory.swap.native@1.0-service.rc starts SwapNativeService only when persist.sys.swapservice.ctrl=true; the captured value is false and swap-installd is stopped. AOSP mmd has separate flag/setup gates; mmd.enabled_aconfig and mmd.setup_complete were not populated in the captured property output. These observations do not prove which Xiaomi framework backend is selected; inspect its consumer before enabling one.

To finish storage expansion, identify the actual ROM consumer, its expected sysfs ABI and storage setup, then validate the implementation, device initialization order, enforcing permissions, write limits and recovery/disable behavior. Kernel backing_dev must be configured before the ZRAM device is initialized. Do not reset or swapoff the running 8 GiB device during this audit. Do not create a storage-backed swap file just to make a switch appear enabled.

No device settings, swap size, backing storage or partitions were changed by this work. A new kernel has not yet been installed. The first stage is a real standard writeback build capability, not completed HyperOS memory expansion.

References:
https://kernel.org/doc/html/v5.19/admin-guide/blockdev/zram.html
Documentation/admin-guide/blockdev/zram.rst
