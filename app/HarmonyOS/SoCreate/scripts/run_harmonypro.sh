#!/usr/bin/env bash
# Build HAP and launch on connected HarmonyOS emulator/device.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../../../.." && pwd)"
PRO_DIR="${REPO_ROOT}/release/HarmonyOS/harmonyOSpro"
DEVECO="/Applications/DevEco-Studio.app/Contents"
export DEVECO_SDK_HOME="${DEVECO_SDK_HOME:-$DEVECO/sdk}"
# Prefer DevEco bundled JBR so hap packaging does not require a system JRE.
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

cd "${PRO_DIR}"
if [[ ! -f local.properties ]] || ! grep -q 'sdk.dir=' local.properties 2>/dev/null; then
  cat > local.properties <<EOF
sdk.dir=${DEVECO_SDK_HOME}
nodejs.dir=${DEVECO}/tools/node
EOF
fi

echo "==> hvigor assembleHap"
hvigorw assembleHap -p product=default -p buildMode=debug --no-daemon

HAP="$(find "${PRO_DIR}/entry/build" -name '*.hap' | head -n 1)"
if [[ -z "${HAP}" ]]; then
  echo "HAP not found" >&2
  exit 1
fi

TARGET="$("${HDC}" list targets | awk 'NF && $1 !~ /Empty/ {print $1; exit}')"
if [[ -z "${TARGET}" ]]; then
  echo "No HarmonyOS device/emulator connected (hdc list targets empty)" >&2
  exit 1
fi

echo "==> Install ${HAP} → ${TARGET}"
"${HDC}" -t "${TARGET}" install "${HAP}"
"${HDC}" -t "${TARGET}" shell aa start -a EntryAbility -b com.example.harmonyospro
echo "Launched com.example.harmonyospro"
