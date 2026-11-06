#!/usr/bin/env bash
set -euo pipefail

# Requires ANDROID_NDK (or ANDROID_NDK_HOME) pointing at the NDK root.
CORE_DIR="$(cd "$(dirname "$0")/.." && pwd)"
REPO_ROOT="$(cd "${CORE_DIR}/../.." && pwd)"
ABI="${1:-arm64-v8a}"
API="${2:-24}"

NDK="${ANDROID_NDK:-${ANDROID_NDK_HOME:-}}"
if [[ -z "${NDK}" ]]; then
  echo "Set ANDROID_NDK or ANDROID_NDK_HOME" >&2
  exit 1
fi

BUILD_DIR="${REPO_ROOT}/out/core/android-${ABI}"
TOOLCHAIN="${NDK}/build/cmake/android.toolchain.cmake"

cmake -S "${CORE_DIR}" -B "${BUILD_DIR}" \
  -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN}" \
  -DANDROID_ABI="${ABI}" \
  -DANDROID_PLATFORM="android-${API}" \
  -DANDROID_STL=c++_static \
  -DCMAKE_BUILD_TYPE=Release \
  -DCORE_BUILD_EXAMPLES=OFF

JOBS="$(sysctl -n hw.ncpu 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"
cmake --build "${BUILD_DIR}" --config Release -j"${JOBS}"

echo "Built: ${BUILD_DIR}/lib/core.a"
