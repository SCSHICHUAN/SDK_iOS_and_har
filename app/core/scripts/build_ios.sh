#!/usr/bin/env bash
# iOS pipeline:
#   1) toolchain 直接编 core.a → release/iOS/a/{iphoneos,iphonesimulator}/
#   2) 打成 Core.xcframework → release/iOS/iOSSDK/
#   3) 安装到 release/iOS/iOSpro/Frameworks/
#   4) 编译并启动 iOSpro 模拟器
set -euo pipefail

CORE_DIR="$(cd "$(dirname "$0")/.." && pwd)"
REPO_ROOT="$(cd "${CORE_DIR}/../.." && pwd)"
FW_CREATE="${REPO_ROOT}/app/iOS/FrameworkCreate"
PACKAGE_SCRIPT="${FW_CREATE}/scripts/package_xcframework.sh"
INSTALL_SCRIPT="${FW_CREATE}/scripts/install_iospro.sh"
IOS_A_OUT="${REPO_ROOT}/release/iOS/a"
IOS_SDK_OUT="${REPO_ROOT}/release/iOS/iOSSDK"
IOS_PRO="${REPO_ROOT}/release/iOS/iOSpro"
HEADERS_DIR="${REPO_ROOT}/src/include"
STAGE_XCF="${IOS_A_OUT}/.stage/Core.xcframework"
JOBS="$(sysctl -n hw.ncpu 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"

# platform: OS64|SIMULATORARM64  →  slice dir name under release/iOS/a
build_core_a() {
  local platform="$1"
  local slice="$2"
  local cmake_dir="${IOS_A_OUT}/.cmake/${slice}"
  local slice_dir="${IOS_A_OUT}/${slice}"

  mkdir -p "${slice_dir}" "${cmake_dir}"

  # 不用 -G Xcode：toolchain + Unix Makefiles 直接出 .a，不生成 xcodeproj
  cmake -S "${CORE_DIR}" -B "${cmake_dir}" \
    -DCMAKE_TOOLCHAIN_FILE="${CORE_DIR}/cmake/toolchains/ios.toolchain.cmake" \
    -DCORE_IOS_PLATFORM="${platform}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCORE_BUILD_EXAMPLES=OFF \
    -DCORE_ENABLE_GL=ON \
    -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY="${slice_dir}" \
    >&2

  cmake --build "${cmake_dir}" --config Release -j"${JOBS}" >&2 || {
    echo "cmake --build failed for slice=${slice}" >&2
    exit 1
  }

  # Prefer freshly built artifact under cmake_dir (do not fall back to stale slice .a).
  local art
  art="$(find "${cmake_dir}" -name 'core.a' | head -n 1 || true)"
  if [[ -z "${art}" || ! -f "${art}" ]]; then
    echo "core.a not found after build for slice=${slice}" >&2
    exit 1
  fi

  # 统一落到 release/iOS/a/<slice>/core.a
  cp -f "${art}" "${slice_dir}/core.a"
  printf '%s\n' "${slice_dir}/core.a"
}

echo "==> [1/4] Build core.a → release/iOS/a/iphoneos"
DEVICE_A="$(build_core_a OS64 iphoneos)"
echo "    ${DEVICE_A}"

echo "==> [2/4] Build core.a → release/iOS/a/iphonesimulator"
SIM_A="$(build_core_a SIMULATORARM64 iphonesimulator)"
echo "    ${SIM_A}"

echo "==> [3/4] Package XCFramework，再强制覆盖 iOSSDK + iOSpro"
chmod +x "${PACKAGE_SCRIPT}" "${INSTALL_SCRIPT}"
rm -rf "${STAGE_XCF}"
mkdir -p "$(dirname "${STAGE_XCF}")"

"${PACKAGE_SCRIPT}" \
  "${DEVICE_A}" \
  "${SIM_A}" \
  "${HEADERS_DIR}" \
  "${STAGE_XCF}"

"${INSTALL_SCRIPT}" "${STAGE_XCF}" "${IOS_SDK_OUT}" "${IOS_PRO}"
echo "    a:    ${IOS_A_OUT}/"
echo "    xc:   ${IOS_SDK_OUT}/Core.xcframework"
echo "    pro:  ${IOS_PRO}/Frameworks/Core.xcframework"

if [[ "${SKIP_RUN:-0}" == "1" ]]; then
  echo "==> [4/4] Skip run (SKIP_RUN=1)"
else
  echo "==> [4/4] Build & run iOSpro on Simulator"
  chmod +x "${FW_CREATE}/scripts/run_iospro.sh"
  "${FW_CREATE}/scripts/run_iospro.sh"
fi

echo "***** 🍺🍺🍺 BUILD SUCCEEDED 🍺🍺🍺 *****"
echo "    iOS pipeline done."
