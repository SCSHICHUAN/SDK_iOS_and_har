# HarmonyOS (OHOS) NDK toolchain for building core.a
# Requires OHOS_NDK (or HOS_SDK_HOME / OHOS_SDK_HOME) pointing at the SDK root.
#
# Usage:
#   cmake -B build-ohos -S . \
#     -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/ohos.toolchain.cmake \
#     -DOHOS_ARCH=arm64-v8a
#
# OHOS_ARCH: arm64-v8a | armeabi-v7a | x86_64

if(DEFINED ENV{OHOS_NDK})
  set(OHOS_NDK "$ENV{OHOS_NDK}")
elseif(DEFINED ENV{HOS_SDK_HOME})
  set(OHOS_NDK "$ENV{HOS_SDK_HOME}")
elseif(DEFINED ENV{OHOS_SDK_HOME})
  set(OHOS_NDK "$ENV{OHOS_SDK_HOME}")
endif()

if(NOT OHOS_NDK)
  message(FATAL_ERROR
    "Set OHOS_NDK (or HOS_SDK_HOME / OHOS_SDK_HOME) to your HarmonyOS NDK/SDK root.")
endif()

if(NOT DEFINED OHOS_ARCH)
  set(OHOS_ARCH "arm64-v8a")
endif()

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

if(OHOS_ARCH STREQUAL "arm64-v8a")
  set(_OHOS_TRIPLE "aarch64-linux-ohos")
  set(CMAKE_SYSTEM_PROCESSOR aarch64)
elseif(OHOS_ARCH STREQUAL "armeabi-v7a")
  set(_OHOS_TRIPLE "arm-linux-ohos")
  set(CMAKE_SYSTEM_PROCESSOR arm)
elseif(OHOS_ARCH STREQUAL "x86_64")
  set(_OHOS_TRIPLE "x86_64-linux-ohos")
  set(CMAKE_SYSTEM_PROCESSOR x86_64)
else()
  message(FATAL_ERROR "Unknown OHOS_ARCH=${OHOS_ARCH}")
endif()

# Common DevEco / OHOS NDK layout.
set(_OHOS_LLVM "${OHOS_NDK}/native/llvm")
if(NOT EXISTS "${_OHOS_LLVM}")
  set(_OHOS_LLVM "${OHOS_NDK}/llvm")
endif()
if(NOT EXISTS "${_OHOS_LLVM}")
  message(FATAL_ERROR "Cannot find OHOS LLVM under ${OHOS_NDK}")
endif()

set(CMAKE_C_COMPILER   "${_OHOS_LLVM}/bin/${_OHOS_TRIPLE}-clang")
set(CMAKE_CXX_COMPILER "${_OHOS_LLVM}/bin/${_OHOS_TRIPLE}-clang++")
set(CMAKE_AR           "${_OHOS_LLVM}/bin/llvm-ar")
set(CMAKE_RANLIB       "${_OHOS_LLVM}/bin/llvm-ranlib")
set(CMAKE_STRIP        "${_OHOS_LLVM}/bin/llvm-strip")

set(CMAKE_FIND_ROOT_PATH "${_OHOS_LLVM}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
