#!/usr/bin/env bash
set -euo pipefail

# Requires OHOS_NDK / HOS_SDK_HOME / OHOS_SDK_HOME.
CORE_DIR="$(cd "$(dirname "$0")/.." && pwd)"
REPO_ROOT="$(cd "${CORE_DIR}/../.." && pwd)"
ARCH="${1:-arm64-v8a}"
BUILD_DIR="${REPO_ROOT}/out/core/ohos-${ARCH}"

cmake -S "${CORE_DIR}" -B "${BUILD_DIR}" \
  -DCMAKE_TOOLCHAIN_FILE="${CORE_DIR}/cmake/toolchains/ohos.toolchain.cmake" \
  -DOHOS_ARCH="${ARCH}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCORE_BUILD_EXAMPLES=OFF

JOBS="$(sysctl -n hw.ncpu 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"
cmake --build "${BUILD_DIR}" --config Release -j"${JOBS}"

echo "Built: ${BUILD_DIR}/lib/core.a"
