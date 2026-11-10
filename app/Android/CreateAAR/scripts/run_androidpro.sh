#!/usr/bin/env bash
# Build and launch androidPro on a connected emulator/device.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../../../.." && pwd)"
PRO_DIR="${REPO_ROOT}/out/Android/androidPro"
SDK_ROOT="${ANDROID_HOME:-${ANDROID_SDK_ROOT:-$HOME/Library/Android/sdk}}"
ADB="${SDK_ROOT}/platform-tools/adb"
STUDIO="/Applications/Android Studio.app/Contents"

# Prefer Android Studio bundled JBR so Gradle does not require a system JDK.
if [[ -z "${JAVA_HOME:-}" ]]; then
  if [[ -x "${STUDIO}/jbr/Contents/Home/bin/java" ]]; then
    export JAVA_HOME="${STUDIO}/jbr/Contents/Home"
  elif [[ -x "${STUDIO}/jbr/bin/java" ]]; then
    export JAVA_HOME="${STUDIO}/jbr"
  fi
fi
if [[ -n "${JAVA_HOME:-}" ]]; then
  export PATH="${JAVA_HOME}/bin:${PATH}"
fi

if [[ ! -x "${ADB}" ]]; then
  ADB="$(command -v adb || true)"
fi
if [[ -z "${ADB}" ]]; then
  echo "adb not found" >&2
  exit 1
fi

cd "${PRO_DIR}"
chmod +x ./gradlew 2>/dev/null || true

echo "==> Gradle assembleDebug (JAVA_HOME=${JAVA_HOME:-unset})"
./gradlew :app:assembleDebug --quiet

APK="${PRO_DIR}/app/build/outputs/apk/debug/app-debug.apk"
if [[ ! -f "${APK}" ]]; then
  echo "APK not found: ${APK}" >&2
  exit 1
fi

echo "==> Waiting for device/emulator"
"${ADB}" wait-for-device
# Wait until boot completed when possible.
boot="$("${ADB}" shell getprop sys.boot_completed 2>/dev/null | tr -d '\r' || true)"
tries=0
while [[ "${boot}" != "1" && ${tries} -lt 60 ]]; do
  sleep 2
  boot="$("${ADB}" shell getprop sys.boot_completed 2>/dev/null | tr -d '\r' || true)"
  tries=$((tries + 1))
done

echo "==> Install & launch"
"${ADB}" install -r "${APK}"
"${ADB}" shell am start -n com.example.androidpro/.MainActivity
echo "Launched com.example.androidpro"
