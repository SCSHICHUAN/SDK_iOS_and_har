#!/usr/bin/env bash
# Cross-compile Assimp static lib for HarmonyOS (OHOS toolchain).
# Usage:
#   ./app/core/scripts/build_assimp_ohos.sh [arch=arm64-v8a] [assimp_src]
#
# Default assimp_src: /Users/shichuan/Documents/code/assimp  or $ASSIMP_SRC
# Output: src/libs/assimp/ohos/<arch>/libassimp.a
set -euo pipefail

CORE_DIR="$(cd "$(dirname "$0")/.." && pwd)"
REPO_ROOT="$(cd "${CORE_DIR}/../.." && pwd)"
ARCH="${1:-arm64-v8a}"
ASSIMP_SRC="${2:-${ASSIMP_SRC:-/Users/shichuan/Documents/code/assimp}}"
TOOLCHAIN="${CORE_DIR}/cmake/toolchains/ohos.toolchain.cmake"
OUT_DIR="${REPO_ROOT}/src/libs/assimp/ohos/${ARCH}"
BUILD_DIR="${REPO_ROOT}/debug/third_party/assimp-ohos/${ARCH}"
JOBS="$(sysctl -n hw.ncpu 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"

if [[ -z "${OHOS_NDK:-${HOS_SDK_HOME:-${OHOS_SDK_HOME:-}}}" ]]; then
  if [[ -d "/Applications/DevEco-Studio.app/Contents/sdk" ]]; then
    export OHOS_NDK="/Applications/DevEco-Studio.app/Contents/sdk"
  fi
fi

if [[ ! -f "${ASSIMP_SRC}/CMakeLists.txt" ]]; then
  echo "Assimp source not found: ${ASSIMP_SRC}" >&2
  echo "Pass path as 2nd arg, or export ASSIMP_SRC=..." >&2
  exit 1
fi
if [[ ! -f "${TOOLCHAIN}" ]]; then
  echo "Missing ${TOOLCHAIN}" >&2
  exit 1
fi

echo "==> Assimp HarmonyOS static lib"
echo "    src:  ${ASSIMP_SRC}"
echo "    arch: ${ARCH}"
echo "    out:  ${OUT_DIR}/libassimp.a"

rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}" "${OUT_DIR}"

cmake -S "${ASSIMP_SRC}" -B "${BUILD_DIR}" \
  -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN}" \
  -DOHOS_ARCH="${ARCH}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_SHARED_LIBS=OFF \
  -DASSIMP_BUILD_TESTS=OFF \
  -DASSIMP_BUILD_SAMPLES=OFF \
  -DASSIMP_BUILD_ASSIMP_TOOLS=OFF \
  -DASSIMP_INSTALL=OFF \
  -DASSIMP_WARNINGS_AS_ERRORS=OFF \
  -DASSIMP_INJECT_DEBUG_POSTFIX=OFF \
  -DASSIMP_BUILD_ZLIB=ON \
  -DASSIMP_ANDROID_JNIIOSYSTEM=OFF

cmake --build "${BUILD_DIR}" --config Release -j"${JOBS}"

ART="$(find "${BUILD_DIR}" -name 'libassimp.a' | head -n 1 || true)"
if [[ -z "${ART}" || ! -f "${ART}" ]]; then
  echo "libassimp.a not found under ${BUILD_DIR}" >&2
  find "${BUILD_DIR}" -name 'libassimp*' | head -20 >&2 || true
  exit 1
fi

cp -f "${ART}" "${OUT_DIR}/libassimp.a"
echo "✅ ${OUT_DIR}/libassimp.a"
ls -lh "${OUT_DIR}/libassimp.a"
file "${OUT_DIR}/libassimp.a" || true
