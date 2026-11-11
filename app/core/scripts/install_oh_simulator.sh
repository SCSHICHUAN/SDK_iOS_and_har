#!/usr/bin/env bash
# Detect an online HarmonyOS emulator, (re)install SDK → harmonyOSpro, then launch.
#
# Usage:
#   ./app/core/scripts/install_oh_simulator.sh
#   SKIP_BUILD=1 ./app/core/scripts/install_oh_simulator.sh
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"
PRO_DIR="${REPO_ROOT}/release/HarmonyOS/harmonyOSpro"
DEVECO="/Applications/DevEco-Studio.app/Contents"
export DEVECO_SDK_HOME="${DEVECO_SDK_HOME:-$DEVECO/sdk}"

if [[ -x "${DEVECO}/jbr/Contents/Home/bin/java" ]]; then
  export JAVA_HOME="${DEVECO}/jbr/Contents/Home"
elif [[ -x "${DEVECO}/jbr/bin/java" ]]; then
  export JAVA_HOME="${DEVECO}/jbr"
fi
export PATH="${JAVA_HOME:+$JAVA_HOME/bin:}$DEVECO/tools/node/bin:$DEVECO/tools/ohpm/bin:$DEVECO/sdk/default/openharmony/toolchains:$DEVECO/tools/hvigor/bin:$PATH"

HDC="$(command -v hdc || true)"
if [[ -z "${HDC}" ]]; then
  HDC="$DEVECO/sdk/default/openharmony/toolchains/hdc"
fi
if [[ ! -x "${HDC}" ]]; then
  echo "hdc not found" >&2
  exit 1
fi

pick_oh_emu() {
  # Prefer localhost emulator that is Connected (not Offline / Empty).
  # hdc list targets -v columns: connectKey  ...  status ...
  "${HDC}" list targets -v 2>/dev/null | awk '
    $0 ~ /Empty/ { next }
    NF < 3 { next }
    {
      key=$1
      # status is usually 3rd field: Connected / Offline
      status=""
      for (i=1; i<=NF; i++) if ($i=="Connected" || $i=="Offline") status=$i
      if (status != "Connected") next
      if (key ~ /^127\.0\.0\.1:/ || key ~ /^localhost:/) { print key; exit }
      # fallback: any Connected target
      if (!any) any=key
    }
    END { if (any) print any }
  '
}

echo "==> Discover HarmonyOS emulator"
TARGET="$(pick_oh_emu || true)"
if [[ -z "${TARGET}" ]]; then
  echo "No online HarmonyOS emulator (hdc Connected on 127.0.0.1:*)." >&2
  echo "Start Mate90Pro (or other) in DevEco Device Manager, then retry." >&2
  "${HDC}" list targets -v >&2 || true
  exit 3
fi
echo "    online: ${TARGET}"

# Quick smoke: shell must work
if ! "${HDC}" -t "${TARGET}" shell echo ok >/dev/null 2>&1; then
  echo "hdc target ${TARGET} listed Connected but shell failed" >&2
  exit 1
fi

if [[ "${SKIP_BUILD:-0}" != "1" ]]; then
  echo "==> Build + install SDK → harmonyOSpro"
  SKIP_RUN=1 "${SCRIPT_DIR}/build_ohos.sh" "${1:-arm64-v8a}"
fi

echo "==> Assemble HAP"
cd "${PRO_DIR}"
if [[ ! -f local.properties ]] || ! grep -q 'sdk.dir=' local.properties 2>/dev/null; then
  cat > local.properties <<EOF
sdk.dir=${DEVECO_SDK_HOME}
nodejs.dir=${DEVECO}/tools/node
EOF
fi
hvigorw assembleHap -p product=default -p buildMode=debug --no-daemon
HAP="$(find "${PRO_DIR}/entry/build" -name '*.hap' | head -n 1)"
if [[ -z "${HAP}" ]]; then
  echo "HAP not found" >&2
  exit 1
fi

echo "==> Install ${HAP} → ${TARGET}"
"${HDC}" -t "${TARGET}" install "${HAP}"
"${HDC}" -t "${TARGET}" shell aa start -a EntryAbility -b com.example.harmonyospro
echo "***** 🍺🍺🍺 BUILD SUCCEEDED 🍺🍺🍺 *****"
echo "    HarmonyOS emulator: ${TARGET}"
