# Lisa Millet integration

Source: MiCode/Xiaomi_Kernel_OpenSource, `dijun-v-oss`, commit
`0b426219a9a24b56c1593ebc84edb8a3f3b897b0`,
`xring-modules/drivers/mihw/millet`. Xiaomi copyright notices and GPL
module declarations are retained. This is a Linux 5.4 port, not an
unchanged copy of the newer GKI integration.

## Kernel changes

- `drivers/misc/millet`: transport, UAPI, signal, TCP UID watch,
  Binder events/state and handshake support. Protocol 31 matches the ROM's
  `ro.millet.netlink`. ARM64 userspace requests are 80 bytes and replies
  144 bytes, excluding the 16-byte netlink header.
- `drivers/android/binder.c`: actual transaction/reply, wait and low async
  space notifications; state queries use Binder's existing proc/inner locks.
  Binder return values, allocation policy and scheduling are unchanged.
- `drivers/misc/Kconfig`, `Makefile`, `.lisa/build.sh`: built-in
  `CONFIG_LISA_MILLET`; compile the new integration before the full build.
- `.lisa/tests/test_millet.py`, workflow: compile the real UAPI and frame
  validators, exercise credentials, truncation, oversized claims,
  forged sender IDs, magic values, enum boundaries and ABI sizes.

The port uses Linux 5.4 freezer helpers and signal tracepoint instead of
newer GKI hooks. It reports events; it does not itself freeze tasks or drop
network traffic. Network callbacks always return `NF_ACCEPT`.

Additional fixes initialize every outbound message, remove kernel task
addresses from replies, initialize all channel locks before callers,
validate the actual skb length and sender port, limit senders to root/system,
fix the stats-reset user read and success counting, and use 5.4 proc file ops.
Reserved MEM requests have no synthetic memory-event implementation.

## ROM changes

None. Existing `/system_ext/bin/millet_monitor`,
`/system_ext/lib64/libmillet_comm.so`, init configuration and protocol
property are retained. Telephony/5G and DRM fixes are outside this change.

## Validation status

Before the port, a read-only device probe confirmed protocol 31 returns
`EPROTONOSUPPORT`, and the service restarted approximately every 9 seconds.
The userspace send/receive code agrees with the above wire sizes.
Compilation and host rejection tests are required before packaging. Device
handshake, real event callbacks, service stability, notifications/calls and
RAM/energy comparison remain required after manual installation. Adding
the transport alone is not evidence that the RAM warning is resolved.

## Shared JNI sender port compatibility (2026-10-19)

The HyperOS `libmiui_service.so` nLoopOnce JNI entry (0x12e14) uses
`create_millet_sock`, which binds the shared socket to PID (type zero).
It then calls `millet_sendmsg` with HANDSHK_TYPE=6. In libmillet_comm.so,
create_millet_sock at 0x4158 passes zero to create_millet_sock_type, while
millet_sendmsg at 0x4230 writes getpid()+type into nlmsg_pid. The real
system_server socket is port 2263, so this command claims 2269.
The previous exact header/sender equality rejected this legitimate command,
including all shared-socket package and Binder queries.

Accept only the observed positive owner offset for MSG_TO_KERN from the
already restricted root/system credentials, after length, magic and enum
validation. LOOPBACK registration still requires exact equality. Neither
registration nor replies use nlmsg_pid as an authority; they use the actual
NETLINK_CB port. Tests cover all six offsets, offset registration rejection,
wrong offsets, unsigned wraparound, invalid owners and shell credentials.
This does not enable Greezer policy, advertise unsupported capabilities or
alter SELinux. Runtime monitor registration still requires device testing.
