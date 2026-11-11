#!/usr/bin/env bash
# Android pipeline (mirror iOS / HarmonyOS):
#   1) repo android.toolchain → release/Android/a/<abi>/core.a
#   2) CreateAAR → release/Android/AndroidSDK/core.aar
#   3) install aar into androidPro
#   4) build & launch androidPro
set -euo pipefail

CORE_DIR="$(cd "$(dirname "$0")/.." && pwd)"
REPO_ROOT="$(cd "${CORE_DIR}/../.." && pwd)"
ABI="${1:-arm64-v8a}"
API="${2:-24}"
CREATE_AAR="${REPO_ROOT}/app/Android/CreateAAR"
PACKAGE_SCRIPT="${CREATE_AAR}/scripts/package_aar.sh"
INSTALL_SCRIPT="${CREATE_AAR}/scripts/install_androidpro.sh"
RUN_SCRIPT="${CREATE_AAR}/scripts/run_androidpro.sh"
A_OUT="${REPO_ROOT}/release/Android/a"
SDK_OUT="${REPO_ROOT}/release/Android/AndroidSDK"
PRO_DIR="${REPO_ROOT}/release/Android/androidPro"
HEADERS_DIR="${REPO_ROOT}/src/include"
TOOLCHAIN="${CORE_DIR}/cmake/toolchains/android.toolchain.cmake"
JOBS="$(sysctl -n hw.ncpu 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"

if [[ ! -f "${TOOLCHAIN}" ]]; then
  echo "Missing repo toolchain: ${TOOLCHAIN}" >&2
  exit 1
fi

echo "==> [1/4] Build core.a → release/Android/a/${ABI}"
CMAKE_DIR="${A_OUT}/.cmake/${ABI}"
SLICE_DIR="${A_OUT}/${ABI}"
mkdir -p "${SLICE_DIR}" "${CMAKE_DIR}"

# Same entry style as iOS/OHOS: repo toolchains/*.cmake (wrapper includes NDK official).
cmake -S "${CORE_DIR}" -B "${CMAKE_DIR}" \
  -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN}" \
  -DANDROID_ABI="${ABI}" \
  -DANDROID_PLATFORM="android-${API}" \
  -DANDROID_STL=c++_static \
  -DCMAKE_BUILD_TYPE=Release \
  -DCORE_ENABLE_GL=ON \
  -DCORE_BUILD_EXAMPLES=OFF \
  -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY="${SLICE_DIR}"

cmake --build "${CMAKE_DIR}" --config Release -j"${JOBS}"

# Always refresh staged core.a from the newest cmake output (avoid stale slice copy).
FOUND="$(find "${CMAKE_DIR}" -name 'core.a' -newer "${CORE_DIR}/CMakeLists.txt" 2>/dev/null | head -n 1 || true)"
if [[ -z "${FOUND}" ]]; then
  FOUND="$(find "${CMAKE_DIR}" -name 'core.a' | head -n 1 || true)"
fi
if [[ -z "${FOUND}" || ! -f "${FOUND}" ]]; then
  echo "core.a not found for abi=${ABI}" >&2
  exit 1
fi
cp -f "${FOUND}" "${SLICE_DIR}/core.a"
DEVICE_A="${SLICE_DIR}/core.a"
echo "    ${DEVICE_A}"

echo "==> [2/4] Package AAR → AndroidSDK/core.aar"
chmod +x "${PACKAGE_SCRIPT}" "${INSTALL_SCRIPT}"
"${PACKAGE_SCRIPT}" "${DEVICE_A}" "${ABI}" "${HEADERS_DIR}" "${SDK_OUT}"

echo "==> [3/4] Force-overwrite AndroidSDK + androidPro"
if [[ ! -f "${SDK_OUT}/core.aar" ]]; then
  echo "missing ${SDK_OUT}/core.aar" >&2
  exit 1
fi
"${INSTALL_SCRIPT}" "${SDK_OUT}/core.aar" "${SDK_OUT}" "${PRO_DIR}" "${HEADERS_DIR}"
echo "    distributable: ${SDK_OUT}/core.aar"
echo "    androidPro libs + AndroidSDK headers overwritten"

if [[ "${SKIP_RUN:-0}" == "1" ]]; then
  echo "==> [4/4] Skip run (SKIP_RUN=1)"
else
  echo "==> [4/4] Build & run androidPro"
  chmod +x "${RUN_SCRIPT}"
  "${RUN_SCRIPT}"
fi

echo "***** 🍺🍺🍺 BUILD SUCCEEDED 🍺🍺🍺 *****"
echo "    a:   ${A_OUT}/${ABI}/core.a"
echo "    aar: ${SDK_OUT}/core.aar"
echo "    pro: ${PRO_DIR}"
