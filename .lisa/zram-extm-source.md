# Lisa ExtM idle-age ABI

Source: MiCode/Xiaomi_Kernel_OpenSource, lisa-r-oss, pinned commit
57ee2b068000ac79caa7c076d0d9bb1a9260e8a4, drivers/block/zram/zram_drv.c
(blob dd1df1f3bb95edc51c950dda5bdec2f3a2a9eb5f).

Adapted behavior: ten idle-mark levels; idle_stat/new_stat eligibility excludes
same-filled, backed and in-flight slots; new all clears idle state; bounded
idle <maximum pages> <minimum age> writeback. Existing huge/idle forms remain.
Original storage/page I/O and in-flight writeback race checks remain in place.
Counter updates use existing slot locks; clearing idle and freeing a slot reset
age. Disksize is sampled under init_lock in modified scanning paths.

The donor flags header is deliberately not copied: this tree uses a 24-bit
object-size field. Four unused bits above its page flags are used, preserving
all other flags. ZRAM_MIUI_EXTM depends on 64BIT and ZRAM_WRITEBACK, and a build
assertion rejects insufficient flags space. No per-page allocation is added.

The request parser is new bounded code, rather than the donor's 32-byte
truncating parser. It rejects overflow, malformed/extra arguments and embedded
NULs without changing output; age values above ten clamp to ten as in donor.
Host tests exercise the same parser/counter header used by the driver.

Not included: MFZ, QPaCE, zgroup, comp_size_stats, backing-device provisioning
or activation of HyperOS properties/services. This is not full memory expansion
until the actual vold/storage path and runtime readback have been verified.
No live swap reset, swapoff or device installation is part of this change.
