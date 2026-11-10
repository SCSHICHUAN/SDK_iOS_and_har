#!/usr/bin/env bash
# Build libcore_napi.so from core.a for HarmonyOS.
# Usage:
#   package_so.sh <core.a> <arch> <headers_dir> <harmony_sdk_out>
set -euo pipefail

CORE_A="${1:?core.a required}"
ARCH="${2:?arch required}"
HEADERS="${3:?headers required}"
SDK_OUT="${4:?HarmonySDK out required}"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
REPO_ROOT="$(cd "${SO_ROOT}/../../.." && pwd)"
TOOLCHAIN="${REPO_ROOT}/app/core/cmake/toolchains/ohos.toolchain.cmake"
BUILD_DIR="${SO_ROOT}/build/${ARCH}"
JOBS="$(sysctl -n hw.ncpu 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"

rm -rf "${BUILD_DIR}"
cmake -S "${SO_ROOT}" -B "${BUILD_DIR}" \
  -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN}" \
  -DOHOS_ARCH="${ARCH}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCORE_A_PATH="${CORE_A}" \
  -DCORE_INCLUDE="${HEADERS}"

cmake --build "${BUILD_DIR}" -j"${JOBS}"

SO_PATH="$(find "${BUILD_DIR}" -name 'libcore_napi.so' | head -n 1)"
if [[ -z "${SO_PATH}" ]]; then
  echo "libcore_napi.so not found" >&2
  exit 1
fi

mkdir -p "${SDK_OUT}/libs/${ARCH}" "${SDK_OUT}/include"
cp "${SO_PATH}" "${SDK_OUT}/libs/${ARCH}/libcore_napi.so"
ditto "${HEADERS}" "${SDK_OUT}/include"
echo "HarmonySDK so: ${SDK_OUT}/libs/${ARCH}/libcore_napi.so"
