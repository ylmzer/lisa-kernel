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

The AOSP service definition documents failure-triggered reboot:
https://android.googlesource.com/platform/packages/modules/Connectivity/+/48646b0d0a5e03461fb872da9c4d3961dbd78fac/netbpfload/netbpfload.mainline.rc
