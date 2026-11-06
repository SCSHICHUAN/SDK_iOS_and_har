#!/usr/bin/env bash
set -euo pipefail

CORE_DIR="$(cd "$(dirname "$0")/.." && pwd)"
REPO_ROOT="$(cd "${CORE_DIR}/../.." && pwd)"
BUILD_DIR="${REPO_ROOT}/out/core/host"

cmake -S "${CORE_DIR}" -B "${BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCORE_BUILD_EXAMPLES=ON

JOBS="$(sysctl -n hw.ncpu 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"
cmake --build "${BUILD_DIR}" --config Release -j"${JOBS}"

echo "Built: ${BUILD_DIR}/lib/core.a"
echo "Run:   ${BUILD_DIR}/core_hello"
