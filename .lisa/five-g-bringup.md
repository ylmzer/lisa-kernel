# A16 to A17 5G regression investigation

## Status and baseline

The user confirms 5G previously worked on this lisa phone with A16 and the
same SIM. A17 remains on LTE. The same SIM obtains 5G in another phone at the
current location; carrier activation and the 5G preferred setting are confirmed.
The installed kernel is dd06b185, with physical display, Wi-Fi, media/call audio,
haptics and LTE browsing confirmed. Hotspot work remains excluded by the user.
No 5G correction has been installed or device-tested.

## Read-only comparison

A16 reference: HYPEROSPRO_LISA_3.0.305.0_A16.
A17 reference: HYPEROSPRO_LISA_4.0.0.32_A17.
Source ROM images were read without mounting or modifying phone partitions.
Private ROM extracts and diagnostic dumps remain ignored and are not published.

The modem, DSP, Bluetooth and devcfg images are byte-identical. 146 selected
vendor radio executables/libraries, modem/operator configuration files and
qcril database files are also byte-identical. This is a selected-file comparison,
not a claim that every ROM or vendor file is identical. DTBO and vendor_boot
images differ; their differences have not been ruled out as contributing factors.

The current framework includes NR in the effective user allowed network types.
Device NR capabilities include NSA and SA; carrier NR availabilities are [1, 2].
Battery saver is off. Current modem registration reports LTE, NR unavailable,
EN-DC unavailable and DC-NR not restricted. These fields do not alone establish
why NR is unavailable.

## Concrete Qualcomm service incompatibility

The current lshal snapshot lists registered Qualcomm HIDL IQtiRadio services
for slot1 and slot2 through version 2.6. Standard Android radio proxies report
HAL 1.5. Server/thread details in lshal are unavailable to the shell; listing
interfaces is not equivalent to a successful NR command or callback.

A16 QtiTelephony contains QtiRadioHidl and a QtiRadioFactory selection path that
falls back to HIDL when AIDL is unavailable. Its enableEndc implementation casts
the existing interface to Qualcomm IQtiRadio 2.3 and calls enableEndc(serial,
enabled). A16 FiveGPowerControllerEx calls ExtTelephonyManager.enableEndc.

A17 QtiTelephony lacks QtiRadioHidl. ExtTelephonyServiceImpl.initQtiRadio and
onMultiSimConfigChanged instantiate QtiRadioAidl for cellular slots; there is
no corresponding legacy HIDL branch in the inspected implementation. The live
ExtTelephonyService dump reports QtiRadioAidl[0/1] mQtiRadio = null.

A17 QtiRadioAidl.enableEndc calls getQtiRadio(). That method explicitly throws
RemoteException("mQtiRadio is null") while the provider is absent. Thus the
current AIDL provider cannot deliver this control call. A17 enable5g is also a
"Not Supported" stub. The latest retained log buffers contain no direct trace
of the failed enableEndc request, so this is static code plus live provider-state
evidence, not a captured request/response failure or a proven sole cause.

A17 retains QcFiveGPowerController.enableEndc through ExtTelephonyManager; do
not incorrectly attribute its normal Qualcomm path to the base controller's
standard NR dual-connectivity API. The base controller does use that standard
API, but A17 RIL guards it with a minimum HAL 1.6 version, beyond the live 1.5
provider. It therefore cannot serve as a compatible fallback on this phone.

## Proposed correction and remaining verification

Restore an actual legacy HIDL backend in the A17 Qualcomm service while keeping
the A17 public Binder interface consistent with its clients. Select AIDL only
when the expected service can connect; otherwise select and connect the real
HIDL provider. Port callback, request token, death/reconnect and version guards
alongside NR/EN-DC operations. Confirm required HIDL Java wrappers remain
available or port the matching wrappers with the service.

This is a userspace service change, outside a kernel Image rebuild. Swapping
only the A16 APK is not a prepared fix: A16 and A17 IExtPhone transaction numbers
and method descriptors differ, so client/server and wrapper compatibility must
be checked together. No modified APK, system image or installer is prepared.
The user provided an extracted CRB ROM project. Its QtiTelephony APK hash
matches the inspected A17 ROM. The project contains binaries, not Java service
sources. Local baksmali disassembly is available for analysis; original CRB ROM
files have not been changed.

Before testing, verify A17 API/client compatibility and unchanged call/LTE paths.
On a user-installed candidate, require a connected real radio backend, successful
NR/EN-DC callbacks, actual NR modem registration/data and physical 5G browsing
with Wi-Fi off. Keep the currently working audio, vibration and Wi-Fi results
as regression checks. Do not fake a 5G icon, availability field or successful
callback. This investigation does not yet establish restored 5G operation.


## CRB project and restoration feasibility (2026-10-05)

A fresh live snapshot confirms both AIDL providers remain null. The phone and
Qualcomm service use extphonelib.jar, qti-telephony-utils.jar and the HIDL wrapper
as shared libraries. extphonelib.jar also differs between A16 and A17. Neither
hyper-telephony-common-ext nor extphonelib is on the captured boot classpath;
do not infer runtime class ownership merely from duplicate DEX definitions.

A limited external invocation audit found 13 references in A17
qti-telephony-common to constructors/methods absent from the inspected A16
extphone definitions. This is not a full Android linker verification: fields,
reflection, all other applications and every shared library have not been
checked. It rejects a claim that the whole A16 client stack is trivially
interchangeable with A17.

Replacing qti-telephony-common with A16 is also unsuitable as a direct fix.
Against the extracted A17 telephony-common definitions, its QtiRIL superclass
constructor signature is absent. Other changed signatures affect data-service
and data-settings constructors, SMS dispatch and component factories. The audit
resolves inspected superclass/interface method inheritance and reports calls
that resolved against A16 but fail against the inspected A17 definitions.
These static incompatibilities do not depend on a 5G coverage assumption.

Both original QtiTelephony APKs pass apksigner verification with the same Xiaomi
certificate. This permits considering an intact original APK only if its full
runtime dependencies are compatible; it does not permit signing a modified APK
with that certificate. The checked local CRB tools expose AOSP test keys rather
than a matching Xiaomi service-signing key. No matching key was supplied and no
signature or permission bypass is part of this work.

There is currently no verified drop-in replacement or installable 5G package.
A compatible A17 HIDL-capable service/client implementation, built and signed
for this ROM, is needed to take the proposed correction to device testing. A
full ROM rebuild with a coherent signing configuration is an alternative larger
scope, not a ready single-APK replacement. The concrete compatibility report is
ready to share with the ROM maintainer. Existing kernel and CRB binaries remain
unchanged; no speculative downgrade has been installed.


## User deferral

The user asks to retain the HIDL adaptation plan and defer 5G work until the PC
connection is moved from mobile data to Wi-Fi. Current priority is Chromium/app
freezes. Do not make radio changes or resume HIDL work without that steering.
