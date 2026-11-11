#!/usr/bin/env bash
# HarmonyOS pipeline (mirror iOS):
#   1) OHOS toolchain → release/HarmonyOS/a/<arch>/core.a
#   2) CreateHAR → release/HarmonyOS/HarmonySDK/core.har
#   3) install har into harmonyOSpro (oh-package file dependency)
#   4) build HAP & launch
set -euo pipefail

CORE_DIR="$(cd "$(dirname "$0")/.." && pwd)"
REPO_ROOT="$(cd "${CORE_DIR}/../.." && pwd)"
ARCH="${1:-arm64-v8a}"
CREATE_HAR="${REPO_ROOT}/app/HarmonyOS/CreateHAR"
PACKAGE_SCRIPT="${CREATE_HAR}/scripts/package_har.sh"
INSTALL_SCRIPT="${CREATE_HAR}/scripts/install_harmonypro.sh"
RUN_SCRIPT="${CREATE_HAR}/scripts/run_harmonypro.sh"
A_OUT="${REPO_ROOT}/release/HarmonyOS/a"
SDK_OUT="${REPO_ROOT}/release/HarmonyOS/HarmonySDK"
PRO_DIR="${REPO_ROOT}/release/HarmonyOS/harmonyOSpro"
HEADERS_DIR="${REPO_ROOT}/src/include"
TOOLCHAIN="${CORE_DIR}/cmake/toolchains/ohos.toolchain.cmake"
JOBS="$(sysctl -n hw.ncpu 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"

# Prefer explicit env; otherwise fall back to DevEco bundled SDK layout.
if [[ -z "${OHOS_NDK:-${HOS_SDK_HOME:-${OHOS_SDK_HOME:-}}}" ]]; then
  if [[ -d "/Applications/DevEco-Studio.app/Contents/sdk" ]]; then
    export OHOS_NDK="/Applications/DevEco-Studio.app/Contents/sdk"
  fi
fi

echo "==> [1/4] Build core.a → release/HarmonyOS/a/${ARCH}"
CMAKE_DIR="${A_OUT}/.cmake/${ARCH}"
SLICE_DIR="${A_OUT}/${ARCH}"
mkdir -p "${SLICE_DIR}" "${CMAKE_DIR}"

cmake -S "${CORE_DIR}" -B "${CMAKE_DIR}" \
  -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN}" \
  -DOHOS_ARCH="${ARCH}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCORE_ENABLE_GL=ON \
  -DCORE_BUILD_EXAMPLES=OFF \
  -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY="${SLICE_DIR}"

cmake --build "${CMAKE_DIR}" --config Release -j"${JOBS}"

# Always refresh staged core.a from the newest cmake output (avoid stale slice copy).
FOUND="$(find "${CMAKE_DIR}" -name 'core.a' | head -n 1 || true)"
if [[ -z "${FOUND}" || ! -f "${FOUND}" ]]; then
  echo "core.a not found for arch=${ARCH}" >&2
  exit 1
fi
cp -f "${FOUND}" "${SLICE_DIR}/core.a"
DEVICE_A="${SLICE_DIR}/core.a"
echo "    ${DEVICE_A}"

echo "==> [2/4] Package HAR → HarmonySDK/core.har"
chmod +x "${PACKAGE_SCRIPT}" "${INSTALL_SCRIPT}"
"${PACKAGE_SCRIPT}" "${DEVICE_A}" "${ARCH}" "${HEADERS_DIR}" "${SDK_OUT}"

echo "==> [3/4] Force-overwrite HarmonySDK + harmonyOSpro"
if [[ ! -f "${SDK_OUT}/core.har" ]]; then
  echo "missing ${SDK_OUT}/core.har" >&2
  exit 1
fi
"${INSTALL_SCRIPT}" "${SDK_OUT}/core.har" "${SDK_OUT}" "${PRO_DIR}" "${HEADERS_DIR}"
echo "    dependency: @sdk/core -> ${SDK_OUT}/core.har"

if [[ "${SKIP_RUN:-0}" == "1" ]]; then
  echo "==> [4/4] Skip run (SKIP_RUN=1)"
else
  echo "==> [4/4] Build & run harmonyOSpro"
  chmod +x "${RUN_SCRIPT}"
  "${RUN_SCRIPT}"
fi

echo "***** 🍺🍺🍺 BUILD SUCCEEDED 🍺🍺🍺 *****"
echo "    a:   ${A_OUT}/${ARCH}/core.a"
echo "    har: ${SDK_OUT}/core.har"
echo "    pro: ${PRO_DIR}"
