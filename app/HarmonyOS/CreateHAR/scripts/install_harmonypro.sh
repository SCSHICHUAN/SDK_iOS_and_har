#!/usr/bin/env bash
# Force-overwrite packaged HAR (+ headers) into HarmonySDK + harmonyOSpro,
# then ensure ohpm dependency is wired (not just copying files).
# Usage:
#   install_harmonypro.sh <core.har> [harmony_sdk_out] [harmony_pro_dir] [headers_dir]
set -euo pipefail

HAR="${1:?core.har required}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../../../.." && pwd)"
SDK_OUT="${2:-${REPO_ROOT}/out/HarmonyOS/HarmonySDK}"
PRO_DIR="${3:-${REPO_ROOT}/out/HarmonyOS/harmonyOSpro}"
HEADERS="${4:-${REPO_ROOT}/src/include}"
DEVECO="/Applications/DevEco-Studio.app/Contents"
OH_PKG="${PRO_DIR}/entry/oh-package.json5"

if [[ ! -f "${HAR}" ]]; then
  echo "core.har not found: ${HAR}" >&2
  exit 1
fi
if [[ ! -d "${PRO_DIR}/entry" ]]; then
  echo "harmonyOSpro entry missing: ${PRO_DIR}/entry" >&2
  exit 1
fi

mkdir -p "${SDK_OUT}/include" "${PRO_DIR}/entry"

HAR_ABS="$(cd "$(dirname "${HAR}")" && pwd)/$(basename "${HAR}")"
SDK_HAR="${SDK_OUT}/core.har"

copy_if_different() {
  local src="$1"
  local dst="$2"
  mkdir -p "$(dirname "${dst}")"
  if [[ "${src}" -ef "${dst}" ]]; then
    return 0
  fi
  cp -f "${src}" "${dst}"
}

# 1) SDK: har + public headers (force refresh)
copy_if_different "${HAR_ABS}" "${SDK_HAR}"
rm -rf "${SDK_OUT}/include"
mkdir -p "${SDK_OUT}/include"
ditto "${HEADERS}" "${SDK_OUT}/include"

echo "overwrite HarmonySDK: ${SDK_HAR}"
echo "overwrite HarmonySDK headers: ${SDK_OUT}/include"

# 2) Pro: drop demo native shortcuts; depend on packaged HAR only
rm -rf "${PRO_DIR}/entry/libs" \
       "${PRO_DIR}/entry/src/main/cpp" \
       "${PRO_DIR}/entry/src/main/ets/types/libcore_napi.d.ts"

# 3) Ensure oh-package dependency is added (DevEco equivalent of “add to project”)
cat > "${OH_PKG}" <<EOF
{
  "name": "entry",
  "version": "1.0.0",
  "description": "harmonyOSpro entry",
  "main": "",
  "author": "",
  "license": "",
  "dependencies": {
    "@sdk/core": "file:../../HarmonySDK/core.har"
  }
}
EOF

cat > "${PRO_DIR}/entry/build-profile.json5" <<EOF
{
  "apiType": "stageMode",
  "buildOption": {
    "resOptions": {
      "copyCodeResource": {
        "enable": false
      }
    }
  },
  "buildOptionSet": [
    {
      "name": "release",
      "arkOptions": {
        "obfuscation": {
          "ruleOptions": {
            "enable": false,
            "files": [
              "./obfuscation-rules.txt"
            ]
          }
        }
      }
    }
  ],
  "targets": [
    {
      "name": "default"
    },
    {
      "name": "ohosTest"
    }
  ]
}
EOF

echo "ohpm: ensured @sdk/core -> file:../../HarmonySDK/core.har"

# 4) Bust ohpm / hvigor / oh_modules caches so new HAR APIs show up
rm -rf "${PRO_DIR}/oh_modules" \
       "${PRO_DIR}/entry/oh_modules" \
       "${PRO_DIR}/entry/build" \
       "${PRO_DIR}/entry/.preview" \
       "${PRO_DIR}/.hvigor" \
       "${PRO_DIR}/entry/oh-package-lock.json5" \
       "${PRO_DIR}/oh-package-lock.json5"

if [[ -x "${DEVECO}/jbr/Contents/Home/bin/java" ]]; then
  export JAVA_HOME="${DEVECO}/jbr/Contents/Home"
elif [[ -x "${DEVECO}/jbr/bin/java" ]]; then
  export JAVA_HOME="${DEVECO}/jbr"
fi
export PATH="${JAVA_HOME:+$JAVA_HOME/bin:}$DEVECO/tools/node/bin:$DEVECO/tools/ohpm/bin:$DEVECO/tools/hvigor/bin:$PATH"

cd "${PRO_DIR}"
ohpm install --all

# Sanity
if ! grep -q 'core_hello_world_string' "${SDK_OUT}/include/core/core.h"; then
  echo "HarmonySDK header stale: ${SDK_OUT}/include/core/core.h" >&2
  exit 1
fi
if ! grep -q '@sdk/core' "${OH_PKG}"; then
  echo "oh-package.json5 missing @sdk/core" >&2
  exit 1
fi
if [[ ! -d "${PRO_DIR}/entry/oh_modules/@sdk/core" && ! -e "${PRO_DIR}/oh_modules/@sdk/core" ]]; then
  # ohpm may nest under entry/oh_modules after install
  echo "warn: @sdk/core not found under oh_modules (check ohpm install output)" >&2
fi

echo "ohpm linked + caches cleared"
echo "overwrite harmonyOSpro: @sdk/core -> ${SDK_OUT}/core.har"
