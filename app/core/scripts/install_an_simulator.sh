#!/usr/bin/env bash
# Detect an online Android emulator, (re)install SDK → androidPro, then launch.
#
# Usage:
#   ./app/core/scripts/install_an_simulator.sh
#   SKIP_BUILD=1 ./app/core/scripts/install_an_simulator.sh
#   BOOT=1 AVD=Medium_Phone_API_37.0 ./app/core/scripts/install_an_simulator.sh
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"
SDK_ROOT="${ANDROID_HOME:-${ANDROID_SDK_ROOT:-$HOME/Library/Android/sdk}}"
ADB="${SDK_ROOT}/platform-tools/adb"
EMU="${SDK_ROOT}/emulator/emulator"
PRO_DIR="${REPO_ROOT}/release/Android/androidPro"
STUDIO="/Applications/Android Studio.app/Contents"
PACKAGE="com.example.androidpro"
ACTIVITY=".MainActivity"

if [[ ! -x "${ADB}" ]]; then
  ADB="$(command -v adb || true)"
fi
if [[ -z "${ADB}" || ! -x "${ADB}" ]]; then
  echo "adb not found under ${SDK_ROOT}/platform-tools" >&2
  exit 1
fi

# Prefer Android Studio JBR for Gradle.
if [[ -z "${JAVA_HOME:-}" ]]; then
  if [[ -x "${STUDIO}/jbr/Contents/Home/bin/java" ]]; then
    export JAVA_HOME="${STUDIO}/jbr/Contents/Home"
  elif [[ -x "${STUDIO}/jbr/bin/java" ]]; then
    export JAVA_HOME="${STUDIO}/jbr"
  fi
fi
[[ -n "${JAVA_HOME:-}" ]] && export PATH="${JAVA_HOME}/bin:${PATH}"

pick_emulator() {
  # Only emulator-* serials that are fully online (device), skip unauthorized/offline.
  "${ADB}" devices | awk '
    NR>1 && $2=="device" && $1 ~ /^emulator-/ { print $1; exit }
  '
}

echo "==> Discover Android emulator"
SERIAL="$(pick_emulator || true)"
if [[ -z "${SERIAL}" ]]; then
  if [[ "${BOOT:-0}" != "1" ]]; then
    echo "No online Android emulator (adb device state=device, serial emulator-*)." >&2
    echo "Start one in Device Manager, or: BOOT=1 AVD=<name> $0" >&2
    "${ADB}" devices -l >&2 || true
    exit 3
  fi
  AVD="${AVD:-Medium_Phone_API_37.0}"
  if [[ ! -x "${EMU}" ]]; then
    echo "emulator binary missing: ${EMU}" >&2
    exit 1
  fi
  echo "    booting AVD: ${AVD}"
  nohup "${EMU}" -avd "${AVD}" -netdelay none -netspeed full >/tmp/core_android_emu.log 2>&1 &
  echo "    waiting for emulator…"
  "${ADB}" wait-for-device
  for _ in $(seq 1 90); do
    SERIAL="$(pick_emulator || true)"
    boot="$("${ADB}" -s "${SERIAL:-none}" shell getprop sys.boot_completed 2>/dev/null | tr -d '\r' || true)"
    if [[ -n "${SERIAL}" && "${boot}" == "1" ]]; then
      break
    fi
    sleep 2
  done
  SERIAL="$(pick_emulator || true)"
  if [[ -z "${SERIAL}" ]]; then
    echo "Emulator failed to come online. See /tmp/core_android_emu.log" >&2
    exit 1
  fi
fi
echo "    online: ${SERIAL}"

if [[ "${SKIP_BUILD:-0}" != "1" ]]; then
  echo "==> Build + install SDK → androidPro"
  SKIP_RUN=1 "${SCRIPT_DIR}/build_android.sh" "${1:-arm64-v8a}" "${2:-24}"
fi

echo "==> Assemble androidPro"
cd "${PRO_DIR}"
chmod +x ./gradlew 2>/dev/null || true
./gradlew :app:assembleDebug --quiet
APK="${PRO_DIR}/app/build/outputs/apk/debug/app-debug.apk"
if [[ ! -f "${APK}" ]]; then
  echo "APK not found: ${APK}" >&2
  exit 1
fi

echo "==> Install & launch on ${SERIAL}"
"${ADB}" -s "${SERIAL}" install -r "${APK}"
"${ADB}" -s "${SERIAL}" shell am start -n "${PACKAGE}/${ACTIVITY}"
echo "***** 🍺🍺🍺 BUILD SUCCEEDED 🍺🍺🍺 *****"
echo "    Android emulator: ${SERIAL}"
