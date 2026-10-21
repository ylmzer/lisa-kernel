#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
MODE="${1:-build}"
case "$MODE" in config-only|zram-only|bootlog-only|vendor-only|build) ;; *) echo "Usage: $0 [config-only|zram-only|bootlog-only|vendor-only|build]" >&2; exit 2;; esac
OUT="$ROOT/out/lisa"
DIST="$ROOT/dist/lisa"
mkdir -p "$OUT" "$DIST"
LLVM_SUFFIX="${LLVM_SUFFIX:--14}"
export LLVM_NM="llvm-nm${LLVM_SUFFIX}"
for tool in "clang${LLVM_SUFFIX}" "ld.lld${LLVM_SUFFIX}" \
  "llvm-ar${LLVM_SUFFIX}" "$LLVM_NM" "llvm-objcopy${LLVM_SUFFIX}" \
  "llvm-objdump${LLVM_SUFFIX}" "llvm-strip${LLVM_SUFFIX}" \
  "llvm-readelf${LLVM_SUFFIX}" "clang++${LLVM_SUFFIX}" \
  aarch64-linux-gnu-gcc arm-linux-gnueabi-gcc; do
  command -v "$tool" >/dev/null || { echo "Missing build tool: $tool" >&2; exit 1; }
done
KERNEL_CC="clang${LLVM_SUFFIX}"
if [ "${USE_CCACHE:-0}" = 1 ]; then
  command -v ccache >/dev/null || { echo 'Missing build tool: ccache' >&2; exit 1; }
  export CCACHE_DIR="${CCACHE_DIR:-$ROOT/out/ccache}"
  export CCACHE_BASEDIR="$ROOT"
  export CCACHE_COMPILERCHECK=content
  ccache --max-size=3G
  KERNEL_CC="ccache $KERNEL_CC"
fi
export ARCH=arm64
export KBUILD_BUILD_USER=ylmzer
# Keep the visible release exact: suppress Git's automatic '+' suffix.
export LOCALVERSION=""
export KBUILD_BUILD_HOST=lisa-builder
export KBUILD_BUILD_VERSION=1
export KBUILD_BUILD_TIMESTAMP="$(git show -s --format=%cD HEAD)"
ARGS=(O="$OUT" ARCH=arm64 TARGET_PRODUCT=lisa LLVM=1 LLVM_IAS=1
  CC="$KERNEL_CC" LD="ld.lld${LLVM_SUFFIX}"
  AR="llvm-ar${LLVM_SUFFIX}" NM="llvm-nm${LLVM_SUFFIX}"
  LLVM_NM="$LLVM_NM"
  OBJCOPY="llvm-objcopy${LLVM_SUFFIX}" OBJDUMP="llvm-objdump${LLVM_SUFFIX}"
  STRIP="llvm-strip${LLVM_SUFFIX}" READELF="llvm-readelf${LLVM_SUFFIX}"
  HOSTCC="clang${LLVM_SUFFIX}" HOSTCXX="clang++${LLVM_SUFFIX}"
  CROSS_COMPILE=aarch64-linux-gnu- CROSS_COMPILE_COMPAT=arm-linux-gnueabi-)
{
  echo "source_commit=$(git rev-parse HEAD)"
  echo "target_product=lisa"
  echo "upstream_commit=7ede20c8692e0e0fc044754429e33cfd1d75c40d"
  echo "mode=$MODE"
  echo "root_support=none"
  echo "boot_log=${BOOT_LOG:-0}"
  echo "zygote_diag=${ZYGOTE_DIAG:-0}"
  echo "build_timestamp=$KBUILD_BUILD_TIMESTAMP"
  "clang${LLVM_SUFFIX}" --version
  "ld.lld${LLVM_SUFFIX}" --version
  dpkg-query -W clang-14 llvm-14 lld-14 2>/dev/null || true
} > "$DIST/build-info.txt"
cp .lisa/configs/tachyon-reference.config "$OUT/.config"
scripts/config --file "$OUT/.config" --set-str LOCALVERSION "-HyperOSPro" \
  -e LISA_MILLET -e ZRAM_WRITEBACK -e ZRAM_MIUI_EXTM -d LOCALVERSION_AUTO -d KSU -d MODULE_FORCE_LOAD -e LISA_NETBPFLOAD_COMPAT \
  -d RMNET -e RMNET_CORE -e RMNET_CTL -d I2C_RTC6226_QCA
if [ "${BOOT_LOG:-0}" = 1 ]; then
  scripts/config --file "$OUT/.config" -e LISA_BOOTLOG
else
  scripts/config --file "$OUT/.config" -d LISA_BOOTLOG
fi
if [ "${ZYGOTE_DIAG:-0}" = 1 ]; then
  scripts/config --file "$OUT/.config" -e LISA_ZYGOTE_DIAG
else
  scripts/config --file "$OUT/.config" -d LISA_ZYGOTE_DIAG
fi
if [ "${IORAP_TEST:-0}" = 1 ]; then
  scripts/config --file "$OUT/.config" -e LISA_IORAP_CAPTURE -e LISA_IORAP_TEST
fi
if [ "${IORAP_INTEGRATION:-0}" = 1 ]; then
  scripts/config --file "$OUT/.config" -e LISA_IORAP_CAPTURE \
    -e LISA_IORAP_LEGACY -e LISA_IORAP_ROM_PTE
  if [ "${IORAP_NATIVE:-0}" = 1 ]; then
    scripts/config --file "$OUT/.config" -d LISA_IORAP_TEST
  else
    scripts/config --file "$OUT/.config" -e LISA_IORAP_TEST
  fi
fi
cp "$OUT/.config" "$DIST/requested.config"
make "${ARGS[@]}" olddefconfig 2>&1 | tee "$DIST/config.log"
cp "$OUT/.config" "$DIST/resolved.config"
KERNEL_RELEASE=$(make -s "${ARGS[@]}" kernelrelease)
[ "$KERNEL_RELEASE" = '5.4.302-HyperOSPro' ] || {
  echo "Unexpected kernel release: $KERNEL_RELEASE" >&2; exit 1;
}
echo "kernel_release=$KERNEL_RELEASE" >> "$DIST/build-info.txt"
grep -qx 'CONFIG_INPUT_AW8624_HAPTIC=y' "$OUT/.config"
grep -qx '# CONFIG_I2C_RTC6226_QCA is not set' "$OUT/.config"
python3 .lisa/config_audit.py "$DIST/requested.config" "$DIST/resolved.config" \
  > "$DIST/config-diff.txt"
python3 .lisa/techpack_audit.py > "$DIST/techpack-config.txt"
if [ "$MODE" = config-only ]; then
  echo 'Config audit completed; this is not a compiled or device-tested kernel.'
  exit 0
fi
if [ "$MODE" = zram-only ]; then
  make -j"${JOBS:-$(nproc)}" "${ARGS[@]}" drivers/block/zram/ mm/zsmalloc.o \
    2>&1 | tee "$DIST/zram-build.log"
  "$LLVM_NM" --defined-only "$OUT/drivers/block/zram/zram_drv.o" > "$DIST/zram-symbols.txt"
  for symbol in backing_dev_store writeback_store writeback_limit_store idle_stat_show new_stat_show new_store; do
    grep -Eq "[[:space:]]${symbol}(\.[[:xdigit:]]{32})?$" "$DIST/zram-symbols.txt" || {
      echo "Missing compiled ZRAM writeback provider: $symbol" >&2; exit 1;
    }
  done
  echo 'ZRAM writeback providers compiled; backing storage and runtime behavior remain unverified.'
  exit 0
fi
if [ "$MODE" = bootlog-only ]; then
  grep -qx 'CONFIG_LISA_BOOTLOG=y' "$OUT/.config"
  make -j"${JOBS:-$(nproc)}" "${ARGS[@]}" drivers/misc/lisa_bootlog.o kernel/sys.o arch/arm64/kernel/syscall.o \
    2>&1 | tee "$DIST/bootlog-build.log"
  echo 'Diagnostic recorder compiled; runtime recording still requires device testing.'
  exit 0
fi
make -j"${JOBS:-$(nproc)}" "${ARGS[@]}" drivers/misc/millet/ drivers/android/binder.o \
  2>&1 | tee "$DIST/millet-build.log"
make -k -j"${JOBS:-$(nproc)}" "${ARGS[@]}" \
  techpack/display/ techpack/audio/ techpack/camera/ techpack/datarmnet/ \
  2>&1 | tee "$DIST/vendor-build.log"
"$LLVM_NM" --defined-only "$OUT/techpack/display/msm/built-in.a" \
  > "$DIST/display-symbols.txt"
for symbol in mi_disp_register_client mi_disp_unregister_client \
  mi_disp_feature_attach_display mi_disp_ioctl mi_dsi_panel_set_disp_param \
  mi_dsi_panel_update_last_bl_level mi_disp_feature_event_notify; do
  grep -Eq "[[:space:]]${symbol}$" "$DIST/display-symbols.txt" || {
    echo "Missing built-in display provider: $symbol" >&2; exit 1;
  }
done
"$LLVM_NM" --defined-only "$OUT/techpack/audio/asoc/codecs/tfa98xx/built-in.a" \
  > "$DIST/audio-amplifier-symbols.txt"
grep -Eq '[[:space:]]tfa98xx_i2c_probe(\.[[:xdigit:]]{32})?$' "$DIST/audio-amplifier-symbols.txt" || {
  echo 'Missing built-in Lisa TFA amplifier provider' >&2; exit 1;
}
if [ "$MODE" = vendor-only ]; then
  echo 'Built-in vendor drivers and display providers verified.'
  exit 0
fi
make -j"${JOBS:-$(nproc)}" "${ARGS[@]}" Image dtbs 2>&1 | tee "$DIST/build.log"
test -s "$OUT/arch/arm64/boot/Image"
cp "$OUT/arch/arm64/boot/Image" "$OUT/vmlinux" "$OUT/System.map" "$DIST/"
grep -aFq 'wlan/qca_cld/qca6750/WCNSS_qcom_cfg.ini' "$DIST/Image" || {
  echo 'Missing QCA6750 firmware configuration namespace' >&2; exit 1;
}
for symbol in wlan_hdd_state_fops rmnet_link_ops rmnet_ctl_register_client \
  tfa98xx_i2c_probe; do
  grep -Eq "[[:space:]]${symbol}$" "$DIST/System.map" || {
    echo "Missing linked device provider: $symbol" >&2; exit 1;
  }
done
if [ "${ZYGOTE_DIAG:-0}" = 1 ]; then
  grep -qx 'CONFIG_LISA_ZYGOTE_DIAG=y' "$DIST/resolved.config"
  grep -aFq 'lisa_zygote_diag: pid=%d syscall=%u result=%ld' "$DIST/Image"
fi
if [ -f "$OUT/Module.symvers" ]; then cp "$OUT/Module.symvers" "$DIST/"; fi
tar -czf "$DIST/dtbs.tar.gz" -C "$OUT/arch/arm64/boot" dts
(cd "$DIST" && sha256sum Image vmlinux System.map resolved.config dtbs.tar.gz > SHA256SUMS)
echo 'Build completed. Device compatibility and Chromium behavior still require testing.'
