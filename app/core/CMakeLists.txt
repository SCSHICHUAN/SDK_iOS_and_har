cmake_minimum_required(VERSION 3.16)

project(core
  VERSION 0.1.0
  LANGUAGES C CXX
)

# Repo layout:
#   src/                 sources + public headers + host example
#   app/core/            this CMake project
#   release/core/<platform>/ build products
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

# OpenGL ES (layout mirrors GL-ARKit/ARKit/GL: camera/shader/mesh/model + C viewer API).
# Consumer app links system GLES + libassimp (+ zlib on iOS).
# GLSL lives in src/gl/shaders/; embedded into core.a at configure time.
# Layout:
#   gl/Interface/           gl_viewer.cpp, gl_platform.h
#   gl/viewport_controls/   camera.h, arcball.h
#   gl/model_load/          mesh/model/animation/animator/debut
#   gl/interaction/         explode/pick/ray
#   gl/                     render/renderer_data/shader, shaders/, models/
option(CORE_ENABLE_GL "Build GLES model viewer into core.a" OFF)
if(CORE_ENABLE_GL)
  set(CORE_GL_SHADERS_DIR "${SDK_SRC_ROOT}/gl/shaders")
  set(CORE_GL_EMBEDDED_SHADERS_H "${CMAKE_CURRENT_BINARY_DIR}/generated/embedded_shaders.h")
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")
  add_custom_command(
    OUTPUT "${CORE_GL_EMBEDDED_SHADERS_H}"
    COMMAND "${CMAKE_COMMAND}"
      -DCORE_GL_SHADERS_DIR=${CORE_GL_SHADERS_DIR}
      -DCORE_GL_EMBEDDED_SHADERS_H=${CORE_GL_EMBEDDED_SHADERS_H}
      -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_gl_shaders.cmake"
    DEPENDS
      "${CORE_GL_SHADERS_DIR}/lighting-vs.vs"
      "${CORE_GL_SHADERS_DIR}/lighting-fs.fs"
      "${CORE_GL_SHADERS_DIR}/lamp-vs.vs"
      "${CORE_GL_SHADERS_DIR}/lamp-fs.fs"
      "${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_gl_shaders.cmake"
    COMMENT "Embed GL shaders into embedded_shaders.h"
    VERBATIM
  )

  target_sources(core PRIVATE
    "${SDK_SRC_ROOT}/gl/Interface/gl_viewer.cpp"
    "${SDK_SRC_ROOT}/gl/renderer_data.cpp"
    "${SDK_SRC_ROOT}/gl/render.cpp"
    "${SDK_SRC_ROOT}/gl/viewport_input.cpp"
    "${SDK_SRC_ROOT}/gl/model_load/model.cpp"
    "${SDK_SRC_ROOT}/gl/model_load/animation.cpp"
    "${SDK_SRC_ROOT}/gl/model_load/animator.cpp"
    "${SDK_SRC_ROOT}/gl/interaction/explode.cpp"
    "${SDK_SRC_ROOT}/gl/interaction/pick.cpp"
    "${CORE_GL_EMBEDDED_SHADERS_H}"
  )
  target_include_directories(core
    PRIVATE
      "${SDK_SRC_ROOT}/gl"
      "${SDK_SRC_ROOT}/gl/Interface"
      "${SDK_SRC_ROOT}/gl/viewport_controls"
      "${SDK_SRC_ROOT}/gl/model_load"
      "${SDK_SRC_ROOT}/gl/interaction"
      "${CMAKE_CURRENT_BINARY_DIR}/generated"
      "${SDK_SRC_ROOT}/libs"
      "${SDK_SRC_ROOT}/libs/assimp/include"
  )
  target_compile_definitions(core PRIVATE CORE_ENABLE_GL=1)
endif()

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
