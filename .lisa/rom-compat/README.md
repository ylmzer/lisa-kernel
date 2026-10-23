# Lisa ROM compatibility: device-validated source fragments

These sources are not installed by the kernel build. They require an exact-ROM vendor image rebuild, normal init policy compilation, unchanged neverallow baseline, complete content/metadata comparison, verified rollback and full partition readback. Do not append policy or flash these files blindly. Original system BootControlClient was not replaced.

FPC fix: exact wakeup_enable sysfs label and only vendor_mfp-daemon getattr/open/write. User confirmed fingerprint unlock; observed 4 accepts, no rejects, one enrolled template, no HAL deaths, Enforcing. No biometric data reset.

Dedicated init oneshot HAL client: genuine HIDL1.1 connected on slot0 (_a), bootable1, successful_before0; actual markBootSuccessful followed by successful-slot readback passed on 2026-10-22. Read-only boot_status_prop permission retains boot-completed guard. No slot switching, fake success or production system client replacement. This does not establish OTA compatibility.

IORAP source135f8882c: actual X cold-launch capture498files/53731events, 13 already-unlinked events explicitly excluded; next cold launch iorapd reported file_sequential preread success498files/206M/341ms. This is not a physical-disk IO or speedup measurement. Android Enforcing and iorapd PID remained stable in this short test. Mutable Crashlytics userlog can still reject recapture with ESTALE; repeated render COLLECT EBUSY, PTE and long-term stability remain open.

Verified kernel config matches CI39631762622 byte-for-byte, all Module.symvers CRCs unchanged, TEST=n, no diagnostic endpoint, required symbols and ZIP/SHA manifests verified. Local binary differs from CI and is not claimed reproducible. Image SHA256935332e0a4604595802f61c3db394ff16c55ff2ac3d9c3bb34ffb7d4d77ee438; installed boot2cf9e27c9fc99b98f5d5c768aa0e7b1901dcbc2daddc6d15f6d1fa498d510e4b; vendor48953ad12b5572abf2b5e5a36b898320956fbb2ea8cd677bca52d6e129eec2de.

## Writable-input kernel788589186: 2026-10-23 device test

Three actual X COLD starts produced valid snapshots444/440/440files,52008/51124/51894events. Explicit writable-event counts1308/974/1122; already-unlinked149/13/22. No STOP ESTALE or quota error in this window. Next launches: genuine iorapd file preread444files/197M/299ms and440files/195M/242ms. Not physical-disk IO or performance proof. Enforcing; iorapd PID2159 unchanged. HAL1.1 actualmark/readback passed again, successful_before1 persisted from preceding boot.

Remaining repeated COLLECT EBUSY diagnosed with live procfs: active PID15940 TGID15940 UID10361, requested TID17167 namedRenderThread TGID15940 UID10361. Same-process render notification collides with existing collection; no lifecycle fix included in788589186. PTE and long-term stability remain pending.

## Render-attachment kernelfcf62b238 device acceptance

2026-10-23: three X COLD launches all logged verified same-process render attachment, snapshots439/438/445files and51634/52125/52421events. No render EBUSY or STOP ESTALE in this window. Genuine file prereads439files197M373ms and438files199M435ms. Enforcing and iorapd PID2266 stable; actual HAL1.1mark/readback passed again. This is not physical IO/performance proof. SamePID16798 HOT resume exposed a separate IOCTL_Pte_Preread ESTALE (errno116); PTE identity guards remain intact and unresolved. Long-term stability pending.

## PTE diagnostic6f45f1369 device result

2026-10-23: actual samePID15460 HOT resume rejected at stage=vma chunk669: missing0/start_changed0/end_changed1/anon_changed0/file0/unreadable0. Snapshot1162chunks48721pages. End direction not recorded; growth vs shrink not established. Enforcing iorapd2066, HALmark/readback passed. Cold snapshots436/443files passed but one of two render notifications again returned EBUSY; earlier3-start success is not universal acceptance. No guard was changed in diagnostic6f. PTE and intermittent startup render attachment still need fixes.
