#!/usr/bin/env bash
# One-shot: build + package + install for iOS / Android / HarmonyOS.
# Default skips launching Pro (no emulator required). Set RUN_PRO=1 to also run.
#
# Usage:
#   ./app/core/scripts/build_all_install.sh
#   RUN_PRO=1 ./app/core/scripts/build_all_install.sh
#   ./app/core/scripts/build_all_install.sh arm64-v8a 24   # Android abi/api also used as OHOS arch
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ABI="${1:-arm64-v8a}"
API="${2:-24}"

if [[ "${RUN_PRO:-0}" == "1" ]]; then
  export SKIP_RUN=0
else
  export SKIP_RUN=1
fi

echo "======== build_all_install ========"
echo "ABI/ARCH=${ABI}  API=${API}  SKIP_RUN=${SKIP_RUN}"
echo

echo "-------- iOS --------"
"${SCRIPT_DIR}/build_ios.sh"
echo

echo "-------- Android --------"
"${SCRIPT_DIR}/build_android.sh" "${ABI}" "${API}"
echo

echo "-------- HarmonyOS --------"
"${SCRIPT_DIR}/build_ohos.sh" "${ABI}"
echo

echo "✅ build_all_install done."
echo "    iOS:     release/iOS/iOSSDK + iOSpro"
echo "    Android: release/Android/AndroidSDK + androidPro"
echo "    OHOS:    release/HarmonyOS/HarmonySDK + harmonyOSpro"
if [[ "${SKIP_RUN}" == "1" ]]; then
  echo "    (Pro not launched; RUN_PRO=1 to also run)"
fi
