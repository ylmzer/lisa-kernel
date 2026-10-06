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
