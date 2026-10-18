#!/usr/bin/env python3
"""Record config changes and reject loss of essential lisa baseline options."""
import re
import sys
from pathlib import Path

def read(path):
    result = {}
    for line in Path(path).read_text().splitlines():
        if line.startswith("CONFIG_") and "=" in line:
            key, value = line.split("=", 1)
            result[key] = value
        elif match := re.fullmatch(r"# (CONFIG_\w+) is not set", line):
            result[match[1]] = "n"
    return result

requested, resolved = map(read, sys.argv[1:3])
for key in sorted(requested.keys() | resolved.keys()):
    if requested.get(key) != resolved.get(key):
        print(f"{key}: {requested.get(key, '<absent>')} -> {resolved.get(key, '<absent>')}")
critical = (
    "ARCH_QCOM", "ARCH_YUPIK", "PINCTRL_SM7325", "SCSI_UFS_QCOM",
    "QCOM_KGSL", "TOUCHSCREEN_GOODIX_BRL", "INPUT_AW8624_HAPTIC",
    "CNSS_QCA6750", "SND_SOC", "SECCOMP", "SECCOMP_FILTER",
    "MEMFD_CREATE", "MODVERSIONS", "ZRAM", "ZRAM_WRITEBACK", "BLK_DEV_LOOP", "CRYPTO_LZ4KD",
    "LZ4KD_COMPRESS", "LZ4KD_DECOMPRESS", "ARM64_4K_PAGES", "LISA_NETBPFLOAD_COMPAT",
    "RMNET_CORE", "RMNET_CTL", "QCA_CLD_WLAN",
)
failures = [f"CONFIG_{key}" for key in critical if resolved.get(f"CONFIG_{key}") != "y"]
modules = [key for key, value in resolved.items() if value == "m"]
if modules:
    print("Unexpected modular drivers:", ", ".join(modules), file=sys.stderr)
    failures.extend(modules)
if resolved.get("CONFIG_KSU", "n") != "n":
    failures.append("CONFIG_KSU")
if failures:
    print("Baseline audit failed:", ", ".join(failures), file=sys.stderr)
    sys.exit(1)
print("Essential baseline options retained; runtime compatibility is unverified.")
