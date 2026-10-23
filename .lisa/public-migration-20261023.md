# Validated changes returned to the public repository

The canonical repository is https://github.com/ylmzer/lisa-kernel. The private organization copy was used for isolated development; this main update preserves its source history through validated results90bb0cbfd9e35ffaf2c44c6725053c665682e701. The private repository is not required to build the source.

Included: bounded IORAP file capture/replay, same-process render attachment, writable/unlinked input classification, verified PTE subset advice, owned32MiB residency/checksum test results, exact FPC wakeup policy sources, and the dedicated real HIDL1.1 mark/readback test client. Kernel configuration and ABI match the validated build as documented in .lisa/rom-compat/README.md. Existing build defaults and opt-in flags are retained; this migration does not silently enable experimental interfaces in every build.

VoWiFi configuration experiments, ROM images, private raw device logs, user working files, local backups and full ROM contents are not published. The HIDL helper does not replace the production system BootControlClient. Verified-subset advice is not completion of every original app mapping; long-term stability and comparative performance remain pending.

GitHub Actions workflows require explicit workflow_dispatch. No Actions build is requested by this migration. Source history and local test evidence are preserved without claiming that a quota-blocked GitHub job passed.
