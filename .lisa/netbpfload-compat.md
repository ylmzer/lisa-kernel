# Android 17 NetBpfLoad opening gate

On 4 October 2026 the user installed boot-diagnostic source c18e4724 and
reproduced the Android bootloop. Recovery runs 5.4.302-lisa-bootdiag+.
Six valid snapshots from three boots were recovered from the oops partition.
The first two generations are Android attempts. Both contain:

`NetBpfLoad: Android 25Q4 requires kernel 5.10.`

This appears immediately before orderly volume/APEX shutdown. The installed
tethering APEX 370399999's `etc/netbpfload.35rc` runs netbpfload as root and
sets `reboot_on_failure reboot,bpfloader-failed`. No kernel panic was established.

## Working-kernel evidence

The previously saved Tachyon boot image is a 5.4.302 kernel. Its ARM64 uname
implementation compares task names including `netbpfload`, checks root uid,
and writes `5.10.248` into the returned release field before copying the UTS
structure to userspace. This was confirmed by disassembly, not inferred from
the ordinary `uname -r` output. The other names in Tachyon's same branch are
bpfloader, netd and uprobestats; they are not included in this targeted change.
Binary dumps and disassembly stay in local private storage.

After the user returned to Tachyon during the build, the live successful boot
provided an independent check: ordinary uname reports 5.4.302-tachyon, while
NetBpfLoad logs `kver:50a00f8` (5.10.248), uid 0, API 37/3700 and the same APEX
version 370399999. `bpf.progs_loaded=1` and `sys.boot_completed=1`. The installed
loader's disassembly shows the failing 25Q4 gate returning exit code 7.

## Targeted implementation

`CONFIG_LISA_NETBPFLOAD_COMPAT` defaults off in Kconfig and is enabled by the
Lisa build script. The ARM64 newuname path modifies its local UTS copy only
when the actual uid is global root and the exact task comm is `netbpfload`.
The returned release is 5.10.248. The actual kernel, /proc version, global UTS
namespace, other UTS fields and other processes retain their existing values.
The diagnostic recorder emits a one-time message when this path is used.

This reproduces a confirmed working-kernel compatibility mechanism. It does
not upgrade the kernel to 5.10 or add missing BPF APIs. The loader also uses
the reported version to select programs; verifier failures may become the
next boot blocker. Do not suppress verifier errors or mark programs loaded
without loading them. Android boot and networking remain unverified until
the new package is tested by the user.

CI compiles the actual helper with mocked task/uid inputs and checks the exact
root/name boundaries and preservation of other UTS fields. It also compiles
kernel/sys.o before the full diagnostic build. Source and artifact checks
are not a substitute for device validation.

The full build of `22655521f8608363586de7a524cd7f2383a15441` succeeded:
https://github.com/ylmzer/lisa-kernel/actions/runs/37200397647
The downloaded artifact archive matched GitHub's SHA256. Manifest checksums,
ARM64 Image/ELF headers, final display symbols, installer restrictions and
packaged Image equality passed. Disassembly of the linked newuname confirms
the uid/name branches, exact name including its NUL (LLVM folds strcmp into
overlapping word comparisons), release 5.10.248 and diagnostic marker.

The unchanged CI ZIP was locally renamed `lisa-netbpf-compat.zip` to avoid
confusion with the already-tested recorder-only package. Its SHA256 is
`079589429ab1be5792243b35ec87139ed41b8d6e359dc8f9e0706bc71340ceb9`.
This new package has not yet been installed or tested on the phone.

The AOSP service definition documents failure-triggered reboot:
https://android.googlesource.com/platform/packages/modules/Connectivity/+/48646b0d0a5e03461fb872da9c4d3961dbd78fac/netbpfload/netbpfload.mainline.rc
