# Display candidate device result and network bring-up

On 4 October 2026 the user installed the verified `ab4f5ee5` MIUI display
candidate and confirmed the physical screen appeared successfully. ADB records
Android boot completion and BPF loading. Wi-Fi and mobile data do not work.
Sleep/wake, brightness range and AOD have not yet been physically validated.
The user subsequently also reported phone calls failing. No detailed call
attempt capture is available yet; LTE registration is not proof of working
voice or IMS. Do not claim RMNET fixes voice before device testing.

The deferred driver-load failure unwind previously busy-waited for all
`/dev/wlan` open references. A HAL write invokes that load while holding its
own reference, so it could never return to close the descriptor. That failure
wait is removed; the persistent control endpoint can report the load error
and permit a retry. Normal unload behavior is unchanged.

Wi-Fi HAL reports "Failed to access driver state control param" with ENOENT.
`/dev/wlan` is absent, while the built-in driver only created
`/sys/kernel/boot_wlan/boot_wlan`. Its existing character-device factory was
guarded by MODULE. The built-in initialization now also creates the real
HAL control endpoint without loading firmware early. Both deferred load entry
points share a mutex and skip an already loaded driver; a failed load returns
its error. Short ON writes are bounded instead of reading past the supplied
length. Host tests exercise actual init/load functions and failure cleanup.

The modem registers on LTE but data setup fails with OEM_DCFAILCAUSE_4.
Only `rmnet_ipa0` exists; no RMNET multiplexed links exist. The Qualcomm
datarmnet sources were traversed but their Kbuild unconditionally selected
obj-m, so the all-built-in Image omitted the RMNET core/control providers.
They now have typed Kconfig entries and configuration-selected Kbuild targets.
The build selects RMNET_CORE and RMNET_CTL, disables the alternative mainline
RMNET implementation, and requires linked network symbols. This is a
source/evidence based candidate, not yet proof of working cellular data.

Vendor module CRC rejections are not bypassed. The installer still replaces
only the kernel Image. Raw logs, network details and boot images remain in
ignored private capture storage. Complete CI and device validation are required
before calling network bring-up successful.
