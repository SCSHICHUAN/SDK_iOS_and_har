#!/usr/bin/env bash
# Force-overwrite packaged XCFramework into iOSSDK + iOSpro,
# then ensure it is linked in the Xcode project (not just copied on disk).
# Usage:
#   install_iospro.sh <Core.xcframework> [ios_sdk_out] [ios_pro_dir]
set -euo pipefail

SRC_XCF="${1:?Core.xcframework required}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../../../.." && pwd)"
SDK_OUT="${2:-${REPO_ROOT}/out/iOS/iOSSDK}"
PRO_DIR="${3:-${REPO_ROOT}/out/iOS/iOSpro}"
PBXPROJ="${PRO_DIR}/iOSpro.xcodeproj/project.pbxproj"

if [[ ! -d "${SRC_XCF}" ]]; then
  echo "XCFramework not found: ${SRC_XCF}" >&2
  exit 1
fi

mkdir -p "${SDK_OUT}" "${PRO_DIR}/Frameworks"

# 1) Always wipe then copy — never leave stale Headers / binary.
rm -rf "${SDK_OUT}/Core.xcframework"
ditto "${SRC_XCF}" "${SDK_OUT}/Core.xcframework"

rm -rf "${PRO_DIR}/Frameworks/Core.xcframework"
ditto "${SRC_XCF}" "${PRO_DIR}/Frameworks/Core.xcframework"

echo "overwrite iOSSDK: ${SDK_OUT}/Core.xcframework"
echo "overwrite iOSpro: ${PRO_DIR}/Frameworks/Core.xcframework"

# 2) Ensure Core.xcframework is added to the Xcode project
#    (Frameworks group + Frameworks build phase + FRAMEWORK_SEARCH_PATHS).
if [[ ! -f "${PBXPROJ}" ]]; then
  echo "missing Xcode project: ${PBXPROJ}" >&2
  exit 1
fi

/usr/bin/python3 - "${PBXPROJ}" <<'PY'
import pathlib, re, sys

pbx_path = pathlib.Path(sys.argv[1])
text = pbx_path.read_text()

FILE_REF = "A10000000000000000000002"
BUILD_REF = "A10000000000000000000001"
GROUP_REF = "A10000000000000000000003"

changed = False

# File reference
if f"{FILE_REF} /* Core.xcframework */" not in text:
    block = (
        f"\t\t{FILE_REF} /* Core.xcframework */ = "
        "{isa = PBXFileReference; lastKnownFileType = wrapper.xcframework; "
        'name = Core.xcframework; path = Frameworks/Core.xcframework; '
        'sourceTree = "<group>"; };\n'
    )
    text = text.replace(
        "/* Begin PBXFileReference section */\n",
        "/* Begin PBXFileReference section */\n" + block,
        1,
    )
    changed = True
    print("xcode: added PBXFileReference Core.xcframework")

# Build file (link against target)
if f"{BUILD_REF} /* Core.xcframework in Frameworks */" not in text:
    block = (
        f"\t\t{BUILD_REF} /* Core.xcframework in Frameworks */ = "
        f"{{isa = PBXBuildFile; fileRef = {FILE_REF} /* Core.xcframework */; }};\n"
    )
    text = text.replace(
        "/* Begin PBXBuildFile section */\n",
        "/* Begin PBXBuildFile section */\n" + block,
        1,
    )
    changed = True
    print("xcode: added PBXBuildFile Core.xcframework")

# Frameworks group child
if "A10000000000000000000003 /* Frameworks */" not in text:
    group = (
        f"\t\t{GROUP_REF} /* Frameworks */ = {{\n"
        "\t\t\tisa = PBXGroup;\n"
        "\t\t\tchildren = (\n"
        f"\t\t\t\t{FILE_REF} /* Core.xcframework */,\n"
        "\t\t\t);\n"
        "\t\t\tname = Frameworks;\n"
        '\t\t\tsourceTree = "<group>";\n'
        "\t\t};\n"
    )
    text = text.replace(
        "/* Begin PBXGroup section */\n",
        "/* Begin PBXGroup section */\n" + group,
        1,
    )
    # Insert Frameworks into root group children if missing
    root_pat = re.compile(
        r"(E3CCF26E309D6DAF00D4E62D = \{\n\t\t\tisa = PBXGroup;\n\t\t\tchildren = \((\n(?:\t\t\t\t[^\n]+\n)*)\t\t\t\);)",
        re.M,
    )
    m = root_pat.search(text)
    if m and GROUP_REF not in m.group(2):
        text = (
            text[: m.start(2)]
            + m.group(2)
            + f"\t\t\t\t{GROUP_REF} /* Frameworks */,\n"
            + text[m.end(2) :]
        )
    changed = True
    print("xcode: added Frameworks group")
elif f"{FILE_REF} /* Core.xcframework */" not in text[
    text.find(f"{GROUP_REF} /* Frameworks */") : text.find(f"{GROUP_REF} /* Frameworks */") + 400
]:
    # Group exists but child missing — inject child
    text = text.replace(
        f"{GROUP_REF} /* Frameworks */ = {{\n\t\t\tisa = PBXGroup;\n\t\t\tchildren = (\n",
        f"{GROUP_REF} /* Frameworks */ = {{\n\t\t\tisa = PBXGroup;\n\t\t\tchildren = (\n"
        f"\t\t\t\t{FILE_REF} /* Core.xcframework */,\n",
        1,
    )
    changed = True
    print("xcode: added Core.xcframework into Frameworks group")

# Frameworks build phase membership
phase_pat = re.compile(
    r"(E3CCF274309D6DAF00D4E62D /\* Frameworks \*/ = \{\n"
    r"\t\t\tisa = PBXFrameworksBuildPhase;\n"
    r"\t\t\tbuildActionMask = 2147483647;\n"
    r"\t\t\tfiles = \((\n(?:\t\t\t\t[^\n]+\n)*)\t\t\t\);)",
    re.M,
)
m = phase_pat.search(text)
if m and BUILD_REF not in m.group(2):
    text = (
        text[: m.start(2)]
        + m.group(2)
        + f"\t\t\t\t{BUILD_REF} /* Core.xcframework in Frameworks */,\n"
        + text[m.end(2) :]
    )
    changed = True
    print("xcode: linked Core.xcframework in Frameworks build phase")

# FRAMEWORK_SEARCH_PATHS must include $(PROJECT_DIR)/Frameworks
if "$(PROJECT_DIR)/Frameworks" not in text:
    text = text.replace(
        "FRAMEWORK_SEARCH_PATHS = (\n\t\t\t\t\t\"$(inherited)\",\n",
        "FRAMEWORK_SEARCH_PATHS = (\n\t\t\t\t\t\"$(inherited)\",\n"
        '\t\t\t\t\t"$(PROJECT_DIR)/Frameworks",\n',
    )
    # If setting missing entirely on target configs, inject near PRODUCT_NAME
    if "$(PROJECT_DIR)/Frameworks" not in text:
        text = text.replace(
            "GENERATE_INFOPLIST_FILE = YES;\n",
            "FRAMEWORK_SEARCH_PATHS = (\n"
            '\t\t\t\t\t"$(inherited)",\n'
            '\t\t\t\t\t"$(PROJECT_DIR)/Frameworks",\n'
            "\t\t\t\t);\n"
            "\t\t\t\tGENERATE_INFOPLIST_FILE = YES;\n",
        )
    changed = True
    print("xcode: set FRAMEWORK_SEARCH_PATHS")

# OTHER_LDFLAGS -lc++ (C++ symbols in static Core)
if '"-lc++"' not in text:
    text = text.replace(
        "MARKETING_VERSION = 1.0;\n",
        "MARKETING_VERSION = 1.0;\n"
        "\t\t\t\tOTHER_LDFLAGS = (\n"
        '\t\t\t\t\t"$(inherited)",\n'
        '\t\t\t\t\t"-lc++",\n'
        "\t\t\t\t);\n",
    )
    changed = True
    print("xcode: set OTHER_LDFLAGS -lc++")

if changed:
    pbx_path.write_text(text)
    print(f"xcode: updated {pbx_path}")
else:
    print("xcode: Core.xcframework already linked in project.pbxproj")
PY

# 3) Bust Xcode / Swift module caches so new headers are visible in IDE + build.
rm -rf "${PRO_DIR}/.derivedData"
rm -rf "${HOME}/Library/Developer/Xcode/DerivedData"/iOSpro-* 2>/dev/null || true
# Touch project so open Xcode windows reload file refs
touch "${PRO_DIR}/iOSpro.xcodeproj/project.pbxproj"
touch "${PRO_DIR}/Frameworks/Core.xcframework"

# Sanity: public header must expose the latest C API
HDR="${PRO_DIR}/Frameworks/Core.xcframework/ios-arm64-simulator/Core.framework/Headers/core/core.h"
if [[ ! -f "${HDR}" ]]; then
  HDR="$(find "${PRO_DIR}/Frameworks/Core.xcframework" -path '*/Headers/core/core.h' | head -n 1 || true)"
fi
if [[ -z "${HDR}" || ! -f "${HDR}" ]]; then
  echo "installed XCFramework missing Headers/core/core.h" >&2
  exit 1
fi
if ! grep -q 'core_hello_world_string' "${HDR}"; then
  echo "installed header is stale (missing core_hello_world_string): ${HDR}" >&2
  exit 1
fi

echo "xcode linked + cache cleared"
echo "public header: ${HDR}"
