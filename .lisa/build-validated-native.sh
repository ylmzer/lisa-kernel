#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
MODE="${1:-build}"
case "$MODE" in build|config-only) ;; *) echo 'Usage: build-validated-native.sh [config-only|build]' >&2; exit 2;; esac
python3 .lisa/validate_native_profile.py environment
export LISA_BUILD_PROFILE=validated-native-20261023
export IORAP_NATIVE=1 IORAP_INTEGRATION=1 IORAP_TEST=0 BOOT_LOG=0 ZYGOTE_DIAG=0 USE_CCACHE=0 LLVM_SUFFIX=-14 JOBS=2
export LISA_VALIDATED_BUILD_TIMESTAMP='Fri, 23 Oct 2026 10:49:48 +0000'
mkdir -p out/validated-profile-tools
cat > out/validated-profile-tools/ld.lld-14 <<'LD'
#!/bin/bash
exec /usr/bin/ld.lld-14 --threads=1 --thinlto-jobs=1 "$@"
LD
chmod +x out/validated-profile-tools/ld.lld-14
export PATH="$PWD/out/validated-profile-tools:$PATH"
bash .lisa/build.sh "$MODE"
if [ "$MODE" = build ]; then python3 .lisa/validate_native_profile.py artifacts; fi
