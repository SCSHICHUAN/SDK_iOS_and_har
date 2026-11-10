# HarmonyOS (OHOS) toolchain for building core.a / .so
# Set OHOS_NDK / HOS_SDK_HOME / OHOS_SDK_HOME, or point at DevEco's sdk root.
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
elseif(EXISTS "/Applications/DevEco-Studio.app/Contents/sdk")
  set(OHOS_NDK "/Applications/DevEco-Studio.app/Contents/sdk")
endif()

if(NOT OHOS_NDK)
  message(FATAL_ERROR
    "Set OHOS_NDK (or HOS_SDK_HOME / OHOS_SDK_HOME) to your HarmonyOS NDK/SDK root.")
endif()

if(NOT DEFINED OHOS_ARCH)
  set(OHOS_ARCH "arm64-v8a")
endif()

# Resolve openharmony native root across common DevEco layouts.
set(_OHOS_NATIVE "")
foreach(_cand
    "${OHOS_NDK}/default/openharmony/native"
    "${OHOS_NDK}/openharmony/native"
    "${OHOS_NDK}/native"
    "${OHOS_NDK}")
  if(EXISTS "${_cand}/llvm")
    set(_OHOS_NATIVE "${_cand}")
    break()
  endif()
endforeach()

if(NOT _OHOS_NATIVE)
  message(FATAL_ERROR "Cannot find OHOS native/llvm under ${OHOS_NDK}")
endif()

set(_OHOS_LLVM "${_OHOS_NATIVE}/llvm")
set(_OHOS_SYSROOT "${_OHOS_NATIVE}/sysroot")

if(OHOS_ARCH STREQUAL "arm64-v8a")
  set(_OHOS_TRIPLE "aarch64-unknown-linux-ohos")
  set(_OHOS_LIB_TRIPLE "aarch64-linux-ohos")
  set(CMAKE_SYSTEM_PROCESSOR aarch64)
elseif(OHOS_ARCH STREQUAL "armeabi-v7a")
  set(_OHOS_TRIPLE "armv7-unknown-linux-ohos")
  set(_OHOS_LIB_TRIPLE "arm-linux-ohos")
  set(CMAKE_SYSTEM_PROCESSOR arm)
elseif(OHOS_ARCH STREQUAL "x86_64")
  set(_OHOS_TRIPLE "x86_64-unknown-linux-ohos")
  set(_OHOS_LIB_TRIPLE "x86_64-linux-ohos")
  set(CMAKE_SYSTEM_PROCESSOR x86_64)
else()
  message(FATAL_ERROR "Unknown OHOS_ARCH=${OHOS_ARCH}")
endif()

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_C_COMPILER   "${_OHOS_LLVM}/bin/${_OHOS_TRIPLE}-clang")
set(CMAKE_CXX_COMPILER "${_OHOS_LLVM}/bin/${_OHOS_TRIPLE}-clang++")
set(CMAKE_AR           "${_OHOS_LLVM}/bin/llvm-ar")
set(CMAKE_RANLIB       "${_OHOS_LLVM}/bin/llvm-ranlib")
set(CMAKE_STRIP        "${_OHOS_LLVM}/bin/llvm-strip")

if(EXISTS "${_OHOS_SYSROOT}")
  set(CMAKE_SYSROOT "${_OHOS_SYSROOT}")
  include_directories(SYSTEM "${_OHOS_SYSROOT}/usr/include")
  link_directories("${_OHOS_SYSROOT}/usr/lib/${_OHOS_LIB_TRIPLE}")
endif()

set(CMAKE_FIND_ROOT_PATH "${_OHOS_LLVM}" "${_OHOS_SYSROOT}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
