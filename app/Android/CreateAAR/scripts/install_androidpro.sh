#!/usr/bin/env bash
# Force-overwrite packaged AAR (+ headers) into AndroidSDK + androidPro,
# then ensure Gradle project wires the SDK (not just copying files).
# Usage:
#   install_androidpro.sh <core.aar> [android_sdk_out] [android_pro_dir] [headers_dir]
set -euo pipefail

AAR="${1:?core.aar required}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../../../.." && pwd)"
SDK_OUT="${2:-${REPO_ROOT}/release/Android/AndroidSDK}"
PRO_DIR="${3:-${REPO_ROOT}/release/Android/androidPro}"
HEADERS="${4:-${REPO_ROOT}/src/include}"
CREATE_AAR_CORE="${REPO_ROOT}/app/Android/CreateAAR/project/core"
SETTINGS="${PRO_DIR}/settings.gradle.kts"
APP_GRADLE="${PRO_DIR}/app/build.gradle.kts"

if [[ ! -f "${AAR}" ]]; then
  echo "core.aar not found: ${AAR}" >&2
  exit 1
fi
if [[ ! -d "${CREATE_AAR_CORE}" ]]; then
  echo "CreateAAR core module missing: ${CREATE_AAR_CORE}" >&2
  exit 1
fi

mkdir -p "${SDK_OUT}/include" \
         "${SDK_OUT}/maven/com/example/coresdk/core/1.0.0" \
         "${PRO_DIR}/app/libs"

# Resolve absolute paths so we can skip no-op self-copies (macOS cp errors on identical).
AAR_ABS="$(cd "$(dirname "${AAR}")" && pwd)/$(basename "${AAR}")"
SDK_AAR="${SDK_OUT}/core.aar"
MAVEN_AAR="${SDK_OUT}/maven/com/example/coresdk/core/1.0.0/core-1.0.0.aar"
PRO_AAR="${PRO_DIR}/app/libs/core.aar"

copy_if_different() {
  local src="$1"
  local dst="$2"
  mkdir -p "$(dirname "${dst}")"
  if [[ "${src}" -ef "${dst}" ]]; then
    return 0
  fi
  cp -f "${src}" "${dst}"
}

# 1) SDK: aar + public headers (force refresh)
copy_if_different "${AAR_ABS}" "${SDK_AAR}"
rm -rf "${SDK_OUT}/include"
mkdir -p "${SDK_OUT}/include"
ditto "${HEADERS}" "${SDK_OUT}/include"
copy_if_different "${AAR_ABS}" "${MAVEN_AAR}"
if [[ ! -f "${SDK_OUT}/maven/com/example/coresdk/core/1.0.0/core-1.0.0.pom" ]]; then
  cat > "${SDK_OUT}/maven/com/example/coresdk/core/1.0.0/core-1.0.0.pom" <<'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<project xmlns="http://maven.apache.org/POM/4.0.0"
         xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
         xsi:schemaLocation="http://maven.apache.org/POM/4.0.0 https://maven.apache.org/xsd/maven-4.0.0.xsd">
  <modelVersion>4.0.0</modelVersion>
  <groupId>com.example.coresdk</groupId>
  <artifactId>core</artifactId>
  <version>1.0.0</version>
  <packaging>aar</packaging>
</project>
EOF
fi

# 2) Pro libs copy (Studio / file consumers)
rm -rf "${PRO_DIR}/app/src/main/jniLibs"
rm -f "${PRO_DIR}/app/src/main/java/com/example/androidpro/CoreNative.kt"
copy_if_different "${AAR_ABS}" "${PRO_AAR}"

echo "overwrite AndroidSDK: ${SDK_AAR}"
echo "overwrite AndroidSDK headers: ${SDK_OUT}/include"
echo "overwrite androidPro: ${PRO_AAR}"

# 3) Ensure Gradle project is wired to CreateAAR :coresdk
#    (androidPro builds against the same module that packages the AAR).
if [[ ! -f "${SETTINGS}" || ! -f "${APP_GRADLE}" ]]; then
  echo "androidPro Gradle files missing under ${PRO_DIR}" >&2
  exit 1
fi

/usr/bin/python3 - "${SETTINGS}" "${APP_GRADLE}" "${CREATE_AAR_CORE}" <<'PY'
import pathlib, sys

settings_path = pathlib.Path(sys.argv[1])
app_gradle = pathlib.Path(sys.argv[2])
core_dir = pathlib.Path(sys.argv[3]).resolve()
pro_dir = settings_path.parent.resolve()
rel = pathlib.Path(os_relpath := __import__("os").path.relpath(core_dir, pro_dir)).as_posix()

settings = settings_path.read_text()
changed = False

if 'include(":coresdk")' not in settings:
    settings += f'''

// Dev/IDE: same sources that CreateAAR packages into AndroidSDK/core.aar
include(":coresdk")
project(":coresdk").projectDir =
    file("{rel}")
'''
    changed = True
    print("gradle: added include(:coresdk)")
elif 'project(":coresdk").projectDir' not in settings:
    settings += f'''
project(":coresdk").projectDir =
    file("{rel}")
'''
    changed = True
    print("gradle: set coresdk projectDir")

# Maven local for distributable AAR fallback
if "AndroidSDK/maven" not in settings:
    needle = "repositories {\n        google()\n        mavenCentral()\n"
    insert = (
        "repositories {\n"
        "        google()\n"
        "        mavenCentral()\n"
        "        // Distributable AAR also published here by CreateAAR.\n"
        "        maven {\n"
        '            url = uri("${rootDir}/../AndroidSDK/maven")\n'
        "        }\n"
    )
    if needle in settings:
        settings = settings.replace(needle, insert, 1)
        changed = True
        print("gradle: added AndroidSDK/maven repository")

if changed:
    settings_path.write_text(settings)
else:
    print("gradle: settings.gradle.kts already wires :coresdk")

app = app_gradle.read_text()
if 'implementation(project(":coresdk"))' not in app:
    if "dependencies {" in app:
        app = app.replace(
            "dependencies {",
            'dependencies {\n    // Same module CreateAAR packages into AndroidSDK/core.aar\n'
            '    implementation(project(":coresdk"))\n',
            1,
        )
    else:
        app += '\ndependencies {\n    implementation(project(":coresdk"))\n}\n'
    app_gradle.write_text(app)
    print("gradle: added implementation(project(:coresdk))")
else:
    print("gradle: app already depends on :coresdk")
PY

# 4) Bust Gradle / Studio caches so new JNI / Java API show up
rm -rf "${PRO_DIR}/.gradle" \
       "${PRO_DIR}/app/build" \
       "${PRO_DIR}/build" \
       "${CREATE_AAR_CORE}/build"
# Drop stale IDE caches for this project if present
rm -rf "${PRO_DIR}/.idea/caches" 2>/dev/null || true

# Sanity: packaged headers must expose latest C API
if ! grep -q 'core_hello_world_string' "${SDK_OUT}/include/core/core.h"; then
  echo "AndroidSDK header stale: ${SDK_OUT}/include/core/core.h" >&2
  exit 1
fi
if ! grep -q 'helloWorldString' "${CREATE_AAR_CORE}/src/main/java/com/example/coresdk/CoreNative.java"; then
  echo "CreateAAR Java API missing helloWorldString" >&2
  exit 1
fi

echo "gradle linked + caches cleared"
echo "coresdk module: ${CREATE_AAR_CORE}"
