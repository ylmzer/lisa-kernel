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
