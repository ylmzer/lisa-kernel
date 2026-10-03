# LZ4KD source provenance

The generic compression library, public header and crypto wrapper were copied
without algorithm changes from:

https://github.com/jyxdd/Nidhi-1.2.4-Pure/tree/f9fb57052495080697ccb1635b584630298723f8

Files: `lib/lz4kd/*`, `include/linux/lz4kd.h`, `crypto/lz4kd.c`.
Original Huawei 2022 copyright notices and Dual BSD/GPL or GPL module declarations
are preserved. Device-specific code and root features were not imported.

Local changes connect the codec to Kconfig, Kbuild and ZRAM's backend list.
The device reference config already enables its three configuration symbols;
the config audit now requires them, so support cannot silently disappear.

This codec version targets 4 KiB pages; the config audit requires
`CONFIG_ARM64_4K_PAGES`. The host test executes the actual codec on 4 KiB pages
with multiple data patterns, checks exact round-trips, rejects empty input and
checks bounded output using an allocation of the declared output capacity.
Pattern blocks can fill a smaller buffer successfully; that API behavior is
accepted while memory bounds are still checked.
Address and undefined-behavior sanitizers are enabled, except alignment checks:
the codec intentionally performs unaligned accesses on these target platforms.
Host tests and successful kernel compilation do not validate device ZRAM setup.
