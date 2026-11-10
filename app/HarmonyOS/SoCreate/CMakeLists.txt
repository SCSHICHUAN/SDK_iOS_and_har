cmake_minimum_required(VERSION 3.16)
project(core_napi LANGUAGES C CXX)

# Build libcore_napi.so wrapping prebuilt core.a for HarmonyOS.
# Expected:
#   CORE_A_PATH  - path to core.a
#   CORE_INCLUDE - public headers dir

if(NOT CORE_A_PATH OR NOT EXISTS "${CORE_A_PATH}")
  message(FATAL_ERROR "CORE_A_PATH must point to an existing core.a")
endif()
if(NOT CORE_INCLUDE OR NOT IS_DIRECTORY "${CORE_INCLUDE}")
  message(FATAL_ERROR "CORE_INCLUDE must point to the public include dir")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(core_napi SHARED
  napi/core_napi.cpp
)

target_include_directories(core_napi PRIVATE "${CORE_INCLUDE}")
# ace_napi.z comes from OHOS sysroot (see ohos.toolchain.cmake link_directories).
target_link_libraries(core_napi PRIVATE "${CORE_A_PATH}" ace_napi.z)
set_target_properties(core_napi PROPERTIES
  OUTPUT_NAME "core_napi"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)
