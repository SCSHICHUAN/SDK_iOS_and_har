#!/usr/bin/env bash
# Build core.aar from core.a (CreateAAR packaging step).
# Flow: core.a → libcore_jni.so (SoCreate) → Gradle library AAR
# Usage:
#   package_aar.sh <core.a> <abi> <headers_dir> <android_sdk_out>
set -euo pipefail

CORE_A="${1:?core.a required}"
ABI="${2:?abi required}"
HEADERS="${3:?headers required}"
SDK_OUT="${4:?AndroidSDK out required}"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CREATE_AAR="$(cd "${SCRIPT_DIR}/.." && pwd)"
# CreateAAR → Android → app → repo
REPO_ROOT="$(cd "${CREATE_AAR}/../../.." && pwd)"
PROJECT_DIR="${CREATE_AAR}/project"
CORE_MODULE="${PROJECT_DIR}/core"
CORE_SRC="${CORE_MODULE}/src/main"
SO_CREATE="${REPO_ROOT}/app/Android/SoCreate"
PACKAGE_SO="${SO_CREATE}/scripts/package_so.sh"
SDK_ROOT="${ANDROID_HOME:-${ANDROID_SDK_ROOT:-$HOME/Library/Android/sdk}}"
STUDIO="/Applications/Android Studio.app/Contents"
API="${ANDROID_API:-24}"

resolve_ndk() {
  if [[ -n "${ANDROID_NDK:-}" && -d "${ANDROID_NDK}" ]]; then
    printf '%s\n' "${ANDROID_NDK}"; return
  fi
  if [[ -n "${ANDROID_NDK_HOME:-}" && -d "${ANDROID_NDK_HOME}" ]]; then
    printf '%s\n' "${ANDROID_NDK_HOME}"; return
  fi
  if [[ -d "${SDK_ROOT}/ndk" ]]; then
    local latest
    latest="$(ls -1 "${SDK_ROOT}/ndk" 2>/dev/null | sort -V | tail -n 1 || true)"
    if [[ -n "${latest}" && -d "${SDK_ROOT}/ndk/${latest}" ]]; then
      printf '%s\n' "${SDK_ROOT}/ndk/${latest}"; return
    fi
  fi
  return 1
}

if [[ -z "${JAVA_HOME:-}" ]]; then
  if [[ -x "${STUDIO}/jbr/Contents/Home/bin/java" ]]; then
    export JAVA_HOME="${STUDIO}/jbr/Contents/Home"
  elif [[ -x "${STUDIO}/jbr/bin/java" ]]; then
    export JAVA_HOME="${STUDIO}/jbr"
  fi
fi
if [[ -n "${JAVA_HOME:-}" ]]; then
  export PATH="${JAVA_HOME}/bin:${PATH}"
fi

NDK="$(resolve_ndk || true)"
if [[ -z "${NDK}" ]]; then
  echo "Android NDK not found" >&2
  exit 1
fi
if [[ ! -f "${CORE_A}" ]]; then
  echo "core.a not found: ${CORE_A}" >&2
  exit 1
fi

# Keep SoCreate JNI in sync with CreateAAR/project/core JNI sources.
cp "${CORE_SRC}/cpp/core_jni.cpp" "${SO_CREATE}/jni/core_jni.cpp"

WORKDIR="$(mktemp -d "${TMPDIR:-/tmp}/coresdk_aar.XXXXXX")"
trap 'rm -rf "${WORKDIR}"' EXIT

echo "==> [aar] build libcore_jni.so (SoCreate) NDK=${NDK}"
chmod +x "${PACKAGE_SO}"
"${PACKAGE_SO}" "${CORE_A}" "${ABI}" "${HEADERS}" "${WORKDIR}/sosdk" "${NDK}" "${API}"
SO_PATH="${WORKDIR}/sosdk/jni/${ABI}/libcore_jni.so"
if [[ ! -f "${SO_PATH}" ]]; then
  echo "libcore_jni.so missing" >&2
  exit 1
fi

# Stage prebuilt .so into library jniLibs (no AGP cmake / NDK auto-download).
rm -rf "${CORE_SRC}/jniLibs"
mkdir -p "${CORE_SRC}/jniLibs/${ABI}"
cp "${SO_PATH}" "${CORE_SRC}/jniLibs/${ABI}/libcore_jni.so"

cat > "${PROJECT_DIR}/local.properties" <<EOF
sdk.dir=${SDK_ROOT}
ndk.dir=${NDK}
EOF

# Ensure library build script stays free of externalNativeBuild.
# Pin ndkVersion to the installed NDK so AGP won't try to fetch another one.
NDK_VERSION="$(basename "${NDK}")"
cat > "${CORE_MODULE}/build.gradle.kts" <<EOF
plugins {
    alias(libs.plugins.android.library)
}

android {
    namespace = "com.example.coresdk"
    ndkVersion = "${NDK_VERSION}"
    compileSdk {
        version = release(37)
    }

    defaultConfig {
        minSdk = 24
        consumerProguardFiles("consumer-rules.pro")
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }

    packaging {
        jniLibs {
            useLegacyPackaging = true
        }
    }
}

dependencies {
}
EOF

cd "${PROJECT_DIR}"
chmod +x ./gradlew
echo "==> [aar] Gradle :core:assembleRelease"
./gradlew :core:clean :core:assembleRelease --no-configuration-cache --no-build-cache

AAR="$(find "${CORE_MODULE}/build/outputs/aar" -name '*.aar' | head -n 1 || true)"
if [[ -z "${AAR}" || ! -f "${AAR}" ]]; then
  echo "core.aar not found under ${CORE_MODULE}/build/outputs/aar" >&2
  exit 1
fi

rm -rf "${SDK_OUT}"
mkdir -p "${SDK_OUT}/include" "${SDK_OUT}/jni/${ABI}"
cp "${AAR}" "${SDK_OUT}/core.aar"
cp "${SO_PATH}" "${SDK_OUT}/jni/${ABI}/libcore_jni.so"
ditto "${HEADERS}" "${SDK_OUT}/include"

# Maven layout for androidPro / IDE (implementation("com.example.coresdk:core:1.0.0"))
MAVEN_DIR="${SDK_OUT}/maven/com/example/coresdk/core/1.0.0"
mkdir -p "${MAVEN_DIR}"
cp "${AAR}" "${MAVEN_DIR}/core-1.0.0.aar"
cat > "${MAVEN_DIR}/core-1.0.0.pom" <<'EOF'
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

# Convenience copy into androidPro/app/libs for Studio visibility.
PRO_LIBS="${REPO_ROOT}/out/Android/androidPro/app/libs"
mkdir -p "${PRO_LIBS}"
cp "${AAR}" "${PRO_LIBS}/core.aar"

echo "AndroidSDK aar: ${SDK_OUT}/core.aar"
echo "AndroidSDK maven: ${MAVEN_DIR}/core-1.0.0.aar"
echo "AndroidSDK so:  ${SDK_OUT}/jni/${ABI}/libcore_jni.so"
