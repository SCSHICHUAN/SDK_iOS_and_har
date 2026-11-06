#!/usr/bin/env bash
set -euo pipefail

CORE_DIR="$(cd "$(dirname "$0")/.." && pwd)"
REPO_ROOT="$(cd "${CORE_DIR}/../.." && pwd)"
PLATFORM="${1:-OS64}"  # OS64 | SIMULATORARM64 | SIMULATOR64
BUILD_DIR="${REPO_ROOT}/out/core/ios-${PLATFORM}"

cmake -S "${CORE_DIR}" -B "${BUILD_DIR}" \
  -G Xcode \
  -DCMAKE_TOOLCHAIN_FILE="${CORE_DIR}/cmake/toolchains/ios.toolchain.cmake" \
  -DCORE_IOS_PLATFORM="${PLATFORM}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCORE_BUILD_EXAMPLES=OFF

JOBS="$(sysctl -n hw.ncpu 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"
cmake --build "${BUILD_DIR}" --config Release -j"${JOBS}"

ART="$(find "${BUILD_DIR}" -name 'core.a' | head -n 1 || true)"
echo "Built: ${ART:-${BUILD_DIR}/lib/core.a}"
