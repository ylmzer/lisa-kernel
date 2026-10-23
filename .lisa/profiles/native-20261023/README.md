# Device-validated native profile

Canonical source: https://github.com/ylmzer/lisa-kernel, main. Reference kernel code d324d2db801fd28c2c9bc651ddf99e6248535279, actual boot SHA9c8b458ed41f0e901e31af93fca750a47255a47f3f95dc191c8a0fb0606f089f.

On Ubuntu22.04 amd64 with the exact toolchain recorded in manifest.json:

```sh
bash .lisa/build-validated-native.sh config-only
bash .lisa/build-validated-native.sh build
```

This selects native IORAP capture/legacy/PTE with TEST=n, no boot/zygote diagnostics, clang/LLD14, two make jobs, one linker/ThinLTO job, no ccache, and the reference build timestamp/user/host/version. Package versions plus compiler/tool and LLVM library SHA256 hashes are checked before building. Configuration must be byte-identical before kernel compilation. A complete build must preserve every module symbol/CRC, required linked symbols and absence of the diagnostic endpoint. A mismatch stops the profile; it does not silently install a newer compiler or alter configuration.

The ordinary `.lisa/build.sh` remains a generic build entrypoint. Use the command above or the separate **Build validated Lisa native profile** manual workflow to select this profile. Installing pinned versions requires a package repository retaining those versions; if unavailable, the job fails rather than substituting versions. No paid service or automatic Actions run is required.

The profile outputs `dist/lisa/Image` and verification evidence, not a complete boot.img. Reusing the tested boot's original init/ramdisk/header/AVB layout remains necessary. ROM binary/ramdisk files are not published in this source repository. A checksum-identical Image/boot is not promised: host dependencies, build paths and later source changes can affect bytes. The artifact report explicitly records whether Image matches the reference; passing configuration/ABI checks is not a new device acceptance or performance claim.

AnyKernel3 packaging reference:020dfeccf9d7e962a48400fc94d3e451df92eead. The reference config and Module.symvers are archived here; manifest hashes detect accidental edits. Deliberate kernel/ABI changes require a separately reviewed and tested profile update.
