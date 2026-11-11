#!/usr/bin/env bash
# Build libcore_jni.so from core.a and stage AndroidSDK layout.
# Usage:
#   package_so.sh <core.a> <abi> <headers_dir> <android_sdk_out> <ndk> [api]
set -euo pipefail

CORE_A="${1:?core.a required}"
ABI="${2:?abi required}"
HEADERS="${3:?headers required}"
SDK_OUT="${4:?AndroidSDK out required}"
NDK="${5:?NDK required}"
API="${6:-24}"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${SO_ROOT}/build/${ABI}"
TOOLCHAIN="${NDK}/build/cmake/android.toolchain.cmake"
JOBS="$(sysctl -n hw.ncpu 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"

REPO_ROOT="$(cd "${SO_ROOT}/../../.." && pwd)"
ASSIMP_A="${ASSIMP_A_PATH:-${REPO_ROOT}/src/libs/assimp/android/${ABI}/libassimp.a}"
GL_ARGS=()
if [[ -f "${ASSIMP_A}" ]]; then
  GL_ARGS+=(-DCORE_ENABLE_GL=ON "-DASSIMP_A_PATH=${ASSIMP_A}")
  echo "    GL: link ${ASSIMP_A}"
else
  echo "    GL: skip (missing ${ASSIMP_A})"
fi

rm -rf "${BUILD_DIR}"
cmake -S "${SO_ROOT}" -B "${BUILD_DIR}" \
  -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN}" \
  -DANDROID_ABI="${ABI}" \
  -DANDROID_PLATFORM="android-${API}" \
  -DANDROID_STL=c++_static \
  -DCMAKE_BUILD_TYPE=Release \
  -DCORE_A_PATH="${CORE_A}" \
  -DCORE_INCLUDE="${HEADERS}" \
  "${GL_ARGS[@]}"

cmake --build "${BUILD_DIR}" -j"${JOBS}"

SO_PATH="$(find "${BUILD_DIR}" -name 'libcore_jni.so' | head -n 1)"
if [[ -z "${SO_PATH}" ]]; then
  echo "libcore_jni.so not found" >&2
  exit 1
fi

mkdir -p "${SDK_OUT}/jni/${ABI}" "${SDK_OUT}/include"
cp "${SO_PATH}" "${SDK_OUT}/jni/${ABI}/libcore_jni.so"
ditto "${HEADERS}" "${SDK_OUT}/include"

echo "AndroidSDK so: ${SDK_OUT}/jni/${ABI}/libcore_jni.so"
