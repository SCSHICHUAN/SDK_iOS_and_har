cmake_minimum_required(VERSION 3.16)
project(core_jni LANGUAGES C CXX)

# Build libcore_jni.so that wraps prebuilt core.a for Android.
# Expected cache vars:
#   CORE_A_PATH   - absolute path to core.a
#   CORE_INCLUDE  - path to public headers (contains core/core.h)

if(NOT CORE_A_PATH OR NOT EXISTS "${CORE_A_PATH}")
  message(FATAL_ERROR "CORE_A_PATH must point to an existing core.a")
endif()
if(NOT CORE_INCLUDE OR NOT IS_DIRECTORY "${CORE_INCLUDE}")
  message(FATAL_ERROR "CORE_INCLUDE must point to the public include dir")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(core_jni SHARED
  jni/core_jni.cpp
)

target_include_directories(core_jni PRIVATE "${CORE_INCLUDE}")
target_link_libraries(core_jni PRIVATE "${CORE_A_PATH}" log)
set_target_properties(core_jni PROPERTIES
  OUTPUT_NAME "core_jni"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)
