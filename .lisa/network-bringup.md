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

The saved working Tachyon Image contains the HAL endpoint factory diagnostic
and RMNET control-driver description. The tested display candidate lacks the
factory diagnostic and linked RMNET core/control providers. The parameter
prefix `hdd.fwpath` matches the working reference; no speculative renaming is
needed. These comparisons support the integration changes but do not replace
device testing.

## Verified network candidate

Full CI [37214770327](https://github.com/ylmzer/lisa-kernel/actions/runs/37214770327),
source `4083eb08e6048c0458dc355e6a408a364187afc1`, completed successfully.
Host tests, full vendor/core compilation, configuration audit and final network
symbol checks pass. Local verification checks GitHub's artifact digest, every
manifest checksum, ARM64 Image/ELF, installer/Image equality, the linked
NetBpfLoad boundary, MIUI symbols, WLAN HAL factory diagnostic and RMNET
core/control providers. There are no replacement DTBs, DTBOs or modules.

- Artifact SHA-256: `1445bccdd66fd74f7c88fb6935dd31b57d0cef31d961cd35bf6f9bff91d903e4`.
- ZIP SHA-256: `8e2a7adcf512194d4fea7c97e98cf15d5de17dc28fab0eb63fc51849105eb26f`.
- Image SHA-256: `a6a1b2e9cd8689150fb6fc9f2a7516bed364f2796f064391676bf49f092d9026`.

Local package: `dist/bootdiag/20261004-network-4083eb08/lisa-network-compat.zip`.
The phone copy will be named `lisa-network-compat-4083eb08.zip`. USB ADB is
currently unavailable, so no copy or installation has occurred. Android boot,
display retention, Wi-Fi, cellular data and calls remain unverified for this
new candidate.

## Network candidate device observation

The user installed `4083eb08`. Its `/proc/version` matches the candidate's
build timestamp, Android boot completes and BPF reports loaded. The real
`/dev/wlan` endpoint exists with wifi ownership, RMNET_CTL probes and
`rmnet_data0` is UP. Android ConnectivityService reports LTE INTERNET
validated. Physical mobile-data browsing and calls still require user results.

Wi-Fi progresses past the previous missing endpoint, but its HAL ON write
times out after the ICNSS2 host probe fails. Ueventd reports missing
`wlan/qca_cld/WCNSS_qcom_cfg.ini`: its vendor symlink targets an absent
`/vendor/etc/wifi/WCNSS_qcom_cfg.ini`. The chip-specific
`wlan/qca_cld/qca6750/WCNSS_qcom_cfg.ini` resolves to an existing readable
vendor configuration. No vendor filesystem changes or fake firmware files
were made.

The standalone QCA6750 build now defaults the existing DYNAMIC_SINGLE_CHIP
flag to `qca6750`, selecting the real INI/MAC namespace while retaining
the HAL device name. External module builds and other chip defaults are
unchanged. Final Image verification now requires the actual QCA6750 INI
path. A new complete build and Wi-Fi device test are required.

## Wi-Fi device result and remaining scope

The user installed the fully verified `de5d98fd` candidate and confirmed
Wi-Fi connects. Device logs show the chip-specific INI loads and wlan0
connects. Subsequent audio/haptics candidates retain this configuration.
The user also reports hotspot opens and explicitly requested no further
hotspot work. After installing `dd06b185`, the user confirms two-way call
audio, including microphone operation. LTE framework validation does not
establish user-verified mobile-data browsing.

The user has not seen 5G at the same location with the previous ROM/kernel;
there is no established 5G regression baseline. Leave 5G unverified until a
known coverage/SIM baseline can be tested, without speculative modem changes.
