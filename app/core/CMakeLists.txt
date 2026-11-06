cmake_minimum_required(VERSION 3.16)

project(core
  VERSION 0.1.0
  LANGUAGES C CXX
)

# Repo layout:
#   src/                 sources + public headers + host example
#   app/core/            this CMake project
#   out/core/<platform>/ build products
get_filename_component(SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
set(SDK_SRC_ROOT "${SDK_ROOT}/src")

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Position-independent code helps when the .a is later linked into .so/.dylib.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(core STATIC
  "${SDK_SRC_ROOT}/core.cpp"
)

# Produce core.a (not libcore.a).
set_target_properties(core PROPERTIES
  OUTPUT_NAME "core"
  PREFIX ""
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

target_include_directories(core
  PUBLIC
    $<BUILD_INTERFACE:${SDK_SRC_ROOT}/include>
    $<INSTALL_INTERFACE:include>
)

# Optional demos / host smoke test (not required for the .a itself).
option(CORE_BUILD_EXAMPLES "Build host example that links core.a" ON)
if(CORE_BUILD_EXAMPLES AND NOT CMAKE_CROSSCOMPILING)
  add_executable(core_hello "${SDK_SRC_ROOT}/examples/hello_main.c")
  target_link_libraries(core_hello PRIVATE core)
endif()

install(TARGETS core
  ARCHIVE DESTINATION lib
)
install(DIRECTORY "${SDK_SRC_ROOT}/include/core"
  DESTINATION include
)
