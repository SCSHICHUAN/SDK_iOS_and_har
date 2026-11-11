#!/usr/bin/env bash
# Detect an online iPhone Simulator, (re)install SDK → iOSpro, then launch.
#
# Usage:
#   ./app/core/scripts/install_ios_simulator.sh
#   SKIP_BUILD=1 ./app/core/scripts/install_ios_simulator.sh   # only rebuild/run Pro
#   BOOT=1 ./app/core/scripts/install_ios_simulator.sh          # boot a simulator if none online
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"
RUN_SCRIPT="${REPO_ROOT}/app/iOS/FrameworkCreate/scripts/run_iospro.sh"

pick_sim() {
  xcrun simctl list devices available --json | /usr/bin/python3 -c '
import json, sys, os
boot = os.environ.get("BOOT", "0") == "1"
data = json.load(sys.stdin)
phones = []
for runtime, items in data.get("devices", {}).items():
    for d in items:
        if not d.get("isAvailable", True):
            continue
        if not str(d.get("name", "")).startswith("iPhone"):
            continue
        phones.append(d)
if not phones:
    sys.stderr.write("No available iPhone Simulator found.\n")
    sys.exit(2)
booted = [d for d in phones if d.get("state") == "Booted"]
if booted:
    d = booted[-1]
    print(d["udid"] + "\t" + d["name"] + "\t" + d["state"])
    sys.exit(0)
if not boot:
    sys.stderr.write("No Booted iPhone Simulator. Start one in Simulator.app, or BOOT=1.\n")
    sys.exit(3)
d = phones[-1]
print(d["udid"] + "\t" + d["name"] + "\t" + d["state"])
'
}

echo "==> Discover iOS Simulator"
DEVICE_INFO="$(pick_sim)"
IFS=$'\t' read -r DEVICE_ID DEVICE_NAME DEVICE_STATE <<< "${DEVICE_INFO}"
echo "    online/selected: ${DEVICE_NAME} (${DEVICE_ID}) [${DEVICE_STATE}]"

if [[ "${SKIP_BUILD:-0}" != "1" ]]; then
  echo "==> Build + install SDK → iOSpro"
  SKIP_RUN=1 "${SCRIPT_DIR}/build_ios.sh"
fi

echo "==> Build Pro + install/launch on Simulator"
# Prefer the discovered device: boot if needed, then run_iospro (it re-picks Booted).
if [[ "${DEVICE_STATE}" != "Booted" ]]; then
  xcrun simctl boot "${DEVICE_ID}" || true
  xcrun simctl bootstatus "${DEVICE_ID}" -b
  open -a Simulator --args -CurrentDeviceUDID "${DEVICE_ID}" || true
fi
chmod +x "${RUN_SCRIPT}"
"${RUN_SCRIPT}"
echo "***** 🍺🍺🍺 BUILD SUCCEEDED 🍺🍺🍺 *****"
echo "    iOS Simulator: ${DEVICE_NAME}"
