#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
MODE="${1:-build}"
case "$MODE" in config-only|vendor-only|build) ;; *) echo "Usage: $0 [config-only|vendor-only|build]" >&2; exit 2;; esac
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
export KBUILD_BUILD_HOST=lisa-builder
export KBUILD_BUILD_VERSION=1
export KBUILD_BUILD_TIMESTAMP="$(git show -s --format=%cD HEAD)"
ARGS=(O="$OUT" ARCH=arm64 LLVM=1 LLVM_IAS=1
  CC="$KERNEL_CC" LD="ld.lld${LLVM_SUFFIX}"
  AR="llvm-ar${LLVM_SUFFIX}" NM="llvm-nm${LLVM_SUFFIX}"
  LLVM_NM="$LLVM_NM"
  OBJCOPY="llvm-objcopy${LLVM_SUFFIX}" OBJDUMP="llvm-objdump${LLVM_SUFFIX}"
  STRIP="llvm-strip${LLVM_SUFFIX}" READELF="llvm-readelf${LLVM_SUFFIX}"
  HOSTCC="clang${LLVM_SUFFIX}" HOSTCXX="clang++${LLVM_SUFFIX}"
  CROSS_COMPILE=aarch64-linux-gnu- CROSS_COMPILE_COMPAT=arm-linux-gnueabi-)
{
  echo "source_commit=$(git rev-parse HEAD)"
  echo "upstream_commit=7ede20c8692e0e0fc044754429e33cfd1d75c40d"
  echo "mode=$MODE"
  echo "root_support=none"
  echo "build_timestamp=$KBUILD_BUILD_TIMESTAMP"
  "clang${LLVM_SUFFIX}" --version
  "ld.lld${LLVM_SUFFIX}" --version
  dpkg-query -W clang-14 llvm-14 lld-14 2>/dev/null || true
} > "$DIST/build-info.txt"
cp .lisa/configs/tachyon-reference.config "$OUT/.config"
scripts/config --file "$OUT/.config" --set-str LOCALVERSION "-lisa-ylmzer" \
  -d LOCALVERSION_AUTO -d KSU -d MODULE_FORCE_LOAD
cp "$OUT/.config" "$DIST/requested.config"
make "${ARGS[@]}" olddefconfig 2>&1 | tee "$DIST/config.log"
cp "$OUT/.config" "$DIST/resolved.config"
python3 .lisa/config_audit.py "$DIST/requested.config" "$DIST/resolved.config" \
  > "$DIST/config-diff.txt"
python3 .lisa/techpack_audit.py > "$DIST/techpack-config.txt"
if [ "$MODE" = config-only ]; then
  echo 'Config audit completed; this is not a compiled or device-tested kernel.'
  exit 0
fi
make -j"${JOBS:-$(nproc)}" "${ARGS[@]}" \
  techpack/display/ techpack/audio/ techpack/camera/ 2>&1 | tee "$DIST/vendor-build.log"
"$LLVM_NM" --defined-only "$OUT/techpack/display/msm/msm_drm.o" \
  > "$DIST/display-symbols.txt"
for symbol in mi_disp_register_client mi_disp_unregister_client; do
  grep -Eq "[[:space:]]${symbol}$" "$DIST/display-symbols.txt" || {
    echo "Missing built-in display provider: $symbol" >&2; exit 1;
  }
done
if [ "$MODE" = vendor-only ]; then
  echo 'Built-in vendor drivers and display providers verified.'
  exit 0
fi
make -j"${JOBS:-$(nproc)}" "${ARGS[@]}" Image dtbs 2>&1 | tee "$DIST/build.log"
test -s "$OUT/arch/arm64/boot/Image"
cp "$OUT/arch/arm64/boot/Image" "$OUT/vmlinux" "$OUT/System.map" "$DIST/"
if [ -f "$OUT/Module.symvers" ]; then cp "$OUT/Module.symvers" "$DIST/"; fi
tar -czf "$DIST/dtbs.tar.gz" -C "$OUT/arch/arm64/boot" dts
(cd "$DIST" && sha256sum Image vmlinux System.map resolved.config dtbs.tar.gz > SHA256SUMS)
echo 'Build completed. Device compatibility and Chromium behavior still require testing.'
