#!/usr/bin/env bash
# Clean iOS build / package / Xcode caches (keep iOSpro sources).
set -euo pipefail

CORE_DIR="$(cd "$(dirname "$0")/.." && pwd)"
REPO_ROOT="$(cd "${CORE_DIR}/../.." && pwd)"
IOS_OUT="${REPO_ROOT}/out/iOS"
FW_CREATE="${REPO_ROOT}/app/iOS/FrameworkCreate"
PRO_DIR="${IOS_OUT}/iOSpro"

echo "==> Clean out/iOS/a (core.a + cmake + stage)"
rm -rf "${IOS_OUT}/a"

echo "==> Clean iOSSDK XCFramework"
rm -rf "${IOS_OUT}/iOSSDK/Core.xcframework"

echo "==> Clean FrameworkCreate intermediate frameworks"
rm -rf "${FW_CREATE}/build"

echo "==> Clean iOSpro Frameworks + DerivedData"
rm -rf "${PRO_DIR}/Frameworks/Core.xcframework"
rm -rf "${PRO_DIR}/.derivedData"
rm -rf "${HOME}/Library/Developer/Xcode/DerivedData"/iOSpro-* 2>/dev/null || true

# Drop common Xcode junk under pro (keep sources / xcodeproj)
rm -rf "${PRO_DIR}/build" \
       "${PRO_DIR}/xcuserdata" \
       "${PRO_DIR}/iOSpro.xcodeproj/xcuserdata" \
       "${PRO_DIR}/iOSpro.xcodeproj/project.xcworkspace/xcuserdata" 2>/dev/null || true
find "${PRO_DIR}" -name '.DS_Store' -delete 2>/dev/null || true

echo "✅ iOS cleaned."
echo "    kept: ${PRO_DIR}/iOSpro.xcodeproj + sources"
echo "    rebuild: ${CORE_DIR}/scripts/build_ios.sh"
