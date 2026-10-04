# Lisa MIUI display restoration

Source: MiCode/Xiaomi_Kernel_OpenSource, lisa-r-oss commit
`57ee2b068000ac79caa7c076d0d9bb1a9260e8a4`.

The original GPL display feature files in `techpack/display/msm/mi_disp` and
the original syscall header `techpack/display/include/uapi/display/drm/mi_disp.h`
are restored with their license/copyright headers. Integration restores MIUI
panel configuration, command sets, feature character device, per-display sysfs,
event delivery, power/AOD transitions, ESD IRQ handling and thermal backlight
interfaces. Qualcomm changes outside the OEM integration are retained.

Local adaptations add read-only brightness and max_brightness_clone sysfs
attributes backed by panel state/parsed panel limits, and correct borrowed
payload lifetime in asynchronous feature jobs. Unsupported feature IDs return
EOPNOTSUPP. Ioctl types and lengths are validated. The existing restricted
installer and NetBpfLoad compatibility remain in place.

This is a device-test candidate, not a claim of HyperOS 4 compatibility. The
reference is older than this ROM and cannot establish newer private ABI support.
Hardware delivery must be checked after installation; raw captures remain
private. Full source compilation and final linked-symbol checks are required
before packaging.
