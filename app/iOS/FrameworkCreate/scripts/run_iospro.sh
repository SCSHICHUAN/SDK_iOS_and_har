#!/usr/bin/env bash
# Build iOSpro and launch on an available iPhone Simulator.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../../../.." && pwd)"
PROJECT_DIR="${REPO_ROOT}/release/iOS/iOSpro"
DERIVED="${PROJECT_DIR}/.derivedData"
SCHEME="iOSpro"
BUNDLE_ID="usb-ios.iOSpro"

if [[ ! -d "${PROJECT_DIR}/iOSpro.xcodeproj" ]]; then
  echo "Missing iOSpro.xcodeproj at ${PROJECT_DIR}" >&2
  exit 1
fi

DEVICE_INFO="$(xcrun simctl list devices available --json | /usr/bin/python3 -c '
import json, sys
data = json.load(sys.stdin)
devices = []
for runtime, items in data.get("devices", {}).items():
    if "iOS" not in runtime and "iphonesimulator" not in runtime.lower():
        # still allow all available; filter by iPhone name below
        pass
    for d in items:
        if not d.get("isAvailable", True):
            continue
        name = d.get("name", "")
        if name.startswith("iPhone"):
            devices.append(d)
if not devices:
    sys.stderr.write("No available iPhone Simulator found.\n")
    sys.exit(1)
booted = [d for d in devices if d.get("state") == "Booted"]
selected = booted[-1] if booted else devices[-1]
print(selected["udid"] + "\t" + selected["name"] + "\t" + selected["state"])
')"

IFS=$'\t' read -r DEVICE_ID DEVICE_NAME DEVICE_STATE <<< "${DEVICE_INFO}"
echo "Simulator: ${DEVICE_NAME} (${DEVICE_ID})"

if [[ "${DEVICE_STATE}" != "Booted" ]]; then
  xcrun simctl boot "${DEVICE_ID}" || true
fi
xcrun simctl bootstatus "${DEVICE_ID}" -b
open -a Simulator --args -CurrentDeviceUDID "${DEVICE_ID}" || true

mkdir -p "${DERIVED}"
echo "Building ${SCHEME}…"
xcodebuild \
  -project "${PROJECT_DIR}/iOSpro.xcodeproj" \
  -scheme "${SCHEME}" \
  -configuration Debug \
  -destination "platform=iOS Simulator,id=${DEVICE_ID}" \
  -derivedDataPath "${DERIVED}" \
  -allowProvisioningUpdates \
  CODE_SIGNING_ALLOWED=YES \
  build

APP_PATH="$(find "${DERIVED}/Build/Products" -name 'iOSpro.app' | head -n 1)"
if [[ -z "${APP_PATH}" ]]; then
  echo "iOSpro.app not found under ${DERIVED}" >&2
  exit 1
fi

echo "Installing ${APP_PATH}"
# Overwrite in place (like adb install -r); do not uninstall first.
xcrun simctl install "${DEVICE_ID}" "${APP_PATH}"
# Do not use --console-pty: it attaches and blocks the shell (unlike Android/OHOS).
xcrun simctl launch "${DEVICE_ID}" "${BUNDLE_ID}"

echo "Launched ${BUNDLE_ID} on ${DEVICE_NAME}"
