# Thin Android toolchain entry (mirrors ios/ohos layout).
# Finds the NDK, then includes the official NDK android.toolchain.cmake.
# Do NOT vendor/copy the NDK file into this repo.
#
# Usage:
#   cmake -B build-android -S . \
#     -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/android.toolchain.cmake \
#     -DANDROID_ABI=arm64-v8a \
#     -DANDROID_PLATFORM=android-24
#
# ANDROID_ABI: arm64-v8a | armeabi-v7a | x86_64 | ...
# ANDROID_PLATFORM: android-24 | android-28 | ...

# Resolve NDK root (same search order as former build_android.sh).
if(DEFINED ENV{ANDROID_NDK} AND EXISTS "$ENV{ANDROID_NDK}")
  set(ANDROID_NDK "$ENV{ANDROID_NDK}")
elseif(DEFINED ENV{ANDROID_NDK_HOME} AND EXISTS "$ENV{ANDROID_NDK_HOME}")
  set(ANDROID_NDK "$ENV{ANDROID_NDK_HOME}")
else()
  if(DEFINED ENV{ANDROID_SDK_ROOT})
    set(_ANDROID_SDK "$ENV{ANDROID_SDK_ROOT}")
  elseif(DEFINED ENV{ANDROID_HOME})
    set(_ANDROID_SDK "$ENV{ANDROID_HOME}")
  else()
    set(_ANDROID_SDK "$ENV{HOME}/Library/Android/sdk")
  endif()

  if(EXISTS "${_ANDROID_SDK}/ndk")
    file(GLOB _NDK_CANDIDATES "${_ANDROID_SDK}/ndk/*")
    list(SORT _NDK_CANDIDATES COMPARE NATURAL ORDER DESCENDING)
    foreach(_cand IN LISTS _NDK_CANDIDATES)
      if(IS_DIRECTORY "${_cand}" AND EXISTS "${_cand}/build/cmake/android.toolchain.cmake")
        set(ANDROID_NDK "${_cand}")
        break()
      endif()
    endforeach()
  endif()

  if(NOT ANDROID_NDK AND EXISTS "${_ANDROID_SDK}/ndk-bundle/build/cmake/android.toolchain.cmake")
    set(ANDROID_NDK "${_ANDROID_SDK}/ndk-bundle")
  endif()
endif()

if(NOT ANDROID_NDK)
  message(FATAL_ERROR
    "Android NDK not found. Install NDK, or export ANDROID_NDK / ANDROID_NDK_HOME.")
endif()

set(_ANDROID_OFFICIAL_TOOLCHAIN
  "${ANDROID_NDK}/build/cmake/android.toolchain.cmake")
if(NOT EXISTS "${_ANDROID_OFFICIAL_TOOLCHAIN}")
  message(FATAL_ERROR
    "Missing official toolchain: ${_ANDROID_OFFICIAL_TOOLCHAIN}")
endif()

# Defaults (overridable via -D on the cmake command line).
if(NOT DEFINED ANDROID_ABI)
  set(ANDROID_ABI "arm64-v8a" CACHE STRING "" FORCE)
endif()
if(NOT DEFINED ANDROID_PLATFORM)
  set(ANDROID_PLATFORM "android-24" CACHE STRING "" FORCE)
endif()
if(NOT DEFINED ANDROID_STL)
  set(ANDROID_STL "c++_static" CACHE STRING "" FORCE)
endif()

message(STATUS "Android NDK: ${ANDROID_NDK}")
message(STATUS "Android ABI: ${ANDROID_ABI}, PLATFORM: ${ANDROID_PLATFORM}")

# Hand off to Google's official NDK toolchain (compilers / sysroot / ABI).
include("${_ANDROID_OFFICIAL_TOOLCHAIN}")
