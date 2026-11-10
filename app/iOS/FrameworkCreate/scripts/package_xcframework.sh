#!/usr/bin/env bash
# Build Core.xcframework from device + simulator static libraries.
# Usage:
#   package_xcframework.sh <device_core.a> <simulator_core.a> <public_headers_dir> <output_xcframework>
set -euo pipefail

DEVICE_A="${1:?device core.a required}"
SIM_A="${2:?simulator core.a required}"
HEADERS_DIR="${3:?headers dir required}"
OUTPUT_XCF="${4:?output xcframework path required}"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
FW_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
WORK="${FW_ROOT}/build/frameworks"
DEVICE_FW="${WORK}/iphoneos/Core.framework"
SIM_FW="${WORK}/iphonesimulator/Core.framework"

rm -rf "${WORK}" "${OUTPUT_XCF}"
mkdir -p "${DEVICE_FW}/Headers" "${DEVICE_FW}/Modules"
mkdir -p "${SIM_FW}/Headers" "${SIM_FW}/Modules"

# --- helpers ---------------------------------------------------------------
assemble_framework() {
  local archive="$1"
  local dest="$2"

  # Static archive becomes the framework binary named "Core".
  cp -f "${archive}" "${dest}/Core"
  # Public C API (+ umbrella). Force-refresh headers every package.
  rm -rf "${dest}/Headers"
  mkdir -p "${dest}/Headers" "${dest}/Modules"
  ditto "${HEADERS_DIR}" "${dest}/Headers"
  cp -f "${FW_ROOT}/Headers/Core.h" "${dest}/Headers/Core.h"
  cp -f "${FW_ROOT}/Headers/module.modulemap" "${dest}/Modules/module.modulemap"
  # Bust Xcode module cache: bump CFBundleVersion per package.
  /usr/libexec/PlistBuddy -c "Set :CFBundleVersion $(date +%s)" "${FW_ROOT}/Info.plist" >/dev/null 2>&1 \
    || true
  cp -f "${FW_ROOT}/Info.plist" "${dest}/Info.plist"

  if [[ ! -f "${dest}/Headers/core/core.h" ]]; then
    echo "missing public header after assemble: ${dest}/Headers/core/core.h" >&2
    exit 1
  fi
}

assemble_framework "${DEVICE_A}" "${DEVICE_FW}"
assemble_framework "${SIM_A}" "${SIM_FW}"

xcodebuild -create-xcframework \
  -framework "${DEVICE_FW}" \
  -framework "${SIM_FW}" \
  -output "${OUTPUT_XCF}"

echo "XCFramework: ${OUTPUT_XCF}"
