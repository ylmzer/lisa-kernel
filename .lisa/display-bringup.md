# Compatibility package device result: Android completes, display remains black

On 4 October 2026 the user installed the verified `22655521f` compatibility
package in TWRP. The boot logo and animation passed, then the physical screen
went black. Normal Android ADB remained available. On both the initial boot
and a subsequent user-triggered power-button reboot:

- Actual kernel release is `5.4.302-lisa-bootdiag+`.
- `sys.boot_completed=1` and `bpf.progs_loaded=1`.
- The compatibility diagnostic and loader's `kver:50a00f8` are present.
- SurfaceFlinger runs, zygote runs, boot animation stops.

This validates removal of the observed NetBpfLoad bootloop trigger. It does
not validate visible display, normal networking, or complete ROM compatibility.
Chrome work remains deferred.

## Display observations

Physical short power presses are registered. An awake sample reports display
ON and contains a rendered lockscreen in the SurfaceFlinger screenshot while
the user still reports a black physical screen. Its standard backlight
`brightness` property is zero, despite a nonzero framework brightness.
Another early-boot sample has brightness 4095. These samples are sequential,
not atomic hardware measurements.

`actual_brightness=0` is **not** proof of a dark panel: this tree's
`sde_backlight_device_get_brightness()` unconditionally returns zero.
The backlight HAL is running. No supported wake injection was available:
the ROM rejected the standard input command with INJECT_EVENTS SecurityException.
No permission bypass, service restart, partition write, or agent reboot was used.

## Confirmed interface mismatch; causality still open

The custom tree's mi_display device exposes only `dynamic_fps`. ROM display
logs report missing `/sys/class/mi_display/disp-DSI-0/brightness`,
`max_brightness_clone`, and `/dev/mi_display/disp_feature`. Working Tachyon's
previous logs reference `brightness_clone` below a virtual
`mi_display/disp_feature/disp-DSI-0` device. The saved working Image also
contains the brightness-clone and disp-feature names.

The original Lisa MIUI source provides a substantial display feature subsystem:
[MiCode lisa-r-oss, pinned commit](https://github.com/MiCode/Xiaomi_Kernel_OpenSource/tree/57ee2b068000ac79caa7c076d0d9bb1a9260e8a4/techpack/display/msm/mi_disp).
Its sysfs methods use display-device driver data and real feature handlers.
Brightness-clone is bookkeeping/event state in this source; merely adding a
file named brightness_clone would not restore hardware brightness delivery.
This original revision also does not supply every newer node expected by
the installed ROM. Do not manufacture successful ioctls or placeholder nodes.

The existing custom `dynamic_fps_show()` obtains data with
`to_platform_device(dev)` even though its sysfs device was created separately
with the display as device driver data. This is a source-level defect to fix
when restoring the interface, not an established cause of physical darkness.

The generic FB/DRM config hypothesis was rejected: reference Tachyon and the
custom resolved config both disable FB and DRM_FBDEV_EMULATION and enable
BACKLIGHT_CLASS_DEVICE. Do not enable those unrelated drivers blindly.

Next work must trace brightness delivery, account for the ROM's feature ABI,
and integrate actual driver operations and dependencies before producing a
new test build. All raw logs, screenshots, and boot images remain local in
ignored private capture storage.

## MIUI restoration candidate

`ab4f5ee5324de15461aea673770843aa5afd3a49` restores the Lisa MIUI display
feature driver, its original ioctl header, panel command/configuration support,
power/AOD transitions, event notifications and thermal brightness interfaces.
The read-only brightness attributes use panel state and parsed limits.
[Source provenance and adaptations](mi-display-source.md) are recorded.

CI [37210740264](https://github.com/ylmzer/lisa-kernel/actions/runs/37210740264)
has passed the upstream source check, codec/boot-log/NetBpfLoad tests,
MIUI ioctl ABI and asynchronous payload tests, and complete built-in vendor
driver compilation with required display symbols. Full Image compilation and
packaging also completed successfully. Local checks confirm all 89 panel command/state
indices agree, including dimming, HBM and doze exit. No new candidate has been
installed; visible display success is not yet established.

Comparison of the saved working Image's sysfs attribute descriptors confirms
`max_brightness_clone` is read-only (0444). Its two writable `brightness`
descriptors lead through CFI branch trampolines to the standard backlight and
LED class store functions; their parsing, locking and work-flush operations
match the available source. They do not establish a missing MIUI brightness
store callback. Binary reports remain in ignored private storage.

## Verified candidate package

Downloaded artifact SHA-256 matches GitHub's digest:
`cbaaca910316abbb47f0ea4ff117ae29333d33915356c1297a87ee00e0454896`.
Archive integrity, all manifest checksums, ARM64/configuration checks, the
linked NetBpfLoad boundary, and eight required MIUI display symbols pass.
The installer contains the exact verified Image and no replacement DTB,
DTBO or modules.

Local candidate: `dist/bootdiag/20261004-display-ab4f5ee5/lisa-display-compat.zip`.
ZIP SHA-256: `699dceda364043d1a25ee03429e325b43035030e53db246eb6be02a20b850d71`.
Image SHA-256: `c951ca5fe05aa5fd05e075c571d691477968086fb2bf532e81e4c64ce653a112`.
The intended separately named phone copy is `lisa-display-compat-ab4f5ee5.zip`.
USB ADB is currently unavailable; no copy or installation has occurred.
Android boot and visible display remain unverified for this candidate.
