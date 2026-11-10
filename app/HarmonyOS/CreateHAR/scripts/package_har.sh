#!/usr/bin/env bash
# Build core.har from core.a (CreateHAR packaging step).
# Usage:
#   package_har.sh <core.a> <arch> <headers_dir> <harmony_sdk_out>
set -euo pipefail

CORE_A="${1:?core.a required}"
ARCH="${2:?arch required}"
HEADERS="${3:?headers required}"
SDK_OUT="${4:?HarmonySDK out required}"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CREATE_HAR="$(cd "${SCRIPT_DIR}/.." && pwd)"
PROJECT_DIR="${CREATE_HAR}/project"
CORE_MODULE="${PROJECT_DIR}/core"
CPP_DIR="${CORE_MODULE}/src/main/cpp"
DEVECO="/Applications/DevEco-Studio.app/Contents"
export DEVECO_SDK_HOME="${DEVECO_SDK_HOME:-$DEVECO/sdk}"

if [[ -x "${DEVECO}/jbr/Contents/Home/bin/java" ]]; then
  export JAVA_HOME="${DEVECO}/jbr/Contents/Home"
elif [[ -x "${DEVECO}/jbr/bin/java" ]]; then
  export JAVA_HOME="${DEVECO}/jbr"
fi
export PATH="${JAVA_HOME:+$JAVA_HOME/bin:}$DEVECO/tools/node/bin:$DEVECO/tools/ohpm/bin:$DEVECO/sdk/default/openharmony/toolchains:$DEVECO/tools/hvigor/bin:$PATH"

if [[ ! -f "${CORE_A}" ]]; then
  echo "core.a not found: ${CORE_A}" >&2
  exit 1
fi
if [[ ! -d "${HEADERS}" ]]; then
  echo "headers not found: ${HEADERS}" >&2
  exit 1
fi

# Point HAR native build at the prebuilt static library.
cat > "${CPP_DIR}/core_paths.cmake" <<EOF
set(CORE_A_PATH "${CORE_A}" CACHE FILEPATH "" FORCE)
set(CORE_INCLUDE "${HEADERS}" CACHE PATH "" FORCE)
EOF

# Restrict ABI to the slice we just built.
cat > "${CORE_MODULE}/build-profile.json5" <<EOF
{
  "apiType": "stageMode",
  "buildOption": {
    "externalNativeOptions": {
      "path": "./src/main/cpp/CMakeLists.txt",
      "arguments": "",
      "cppFlags": "",
      "abiFilters": [
        "${ARCH}"
      ]
    }
  },
  "buildOptionSet": [
    {
      "name": "release",
      "arkOptions": {
        "obfuscation": {
          "ruleOptions": {
            "enable": false,
            "files": []
          }
        }
      }
    }
  ],
  "targets": [
    {
      "name": "default"
    }
  ]
}
EOF

cat > "${PROJECT_DIR}/local.properties" <<EOF
sdk.dir=${DEVECO_SDK_HOME}
nodejs.dir=${DEVECO}/tools/node
EOF

cd "${PROJECT_DIR}"
echo "==> assembleHar (@sdk/core)"
hvigorw assembleHar -p module=core -p product=default -p buildMode=release --no-daemon

HAR="$(find "${CORE_MODULE}/build" -name '*.har' | head -n 1 || true)"
if [[ -z "${HAR}" || ! -f "${HAR}" ]]; then
  echo "core.har not found under ${CORE_MODULE}/build" >&2
  exit 1
fi

rm -rf "${SDK_OUT}"
mkdir -p "${SDK_OUT}"
cp "${HAR}" "${SDK_OUT}/core.har"
# Keep a copy of headers next to the HAR for C/C++ consumers if needed.
mkdir -p "${SDK_OUT}/include"
ditto "${HEADERS}" "${SDK_OUT}/include"
echo "HarmonySDK har: ${SDK_OUT}/core.har"
