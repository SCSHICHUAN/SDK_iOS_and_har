#include "napi/native_api.h"

#include "core/core.h"
#include "core/gl_viewer.h"

#include <string>
#include <vector>

static std::string g_modelPath;
static std::vector<uint8_t> g_modelBytes;

extern "C" void core_gl_reload_model_on_surfaces();
extern "C" napi_value GlPresentAll(napi_env env, napi_callback_info info);
extern "C" napi_value GlOrbitAll(napi_env env, napi_callback_info info);
extern "C" napi_value GlOrbitEndAll(napi_env env, napi_callback_info info);
extern "C" napi_value GlDollyAll(napi_env env, napi_callback_info info);
extern "C" napi_value GlLightMoveXAll(napi_env env, napi_callback_info info);
extern "C" napi_value GlLightDragAll(napi_env env, napi_callback_info info);
extern "C" void core_gl_register_xcomponent(napi_env env, napi_value exports);

namespace {

napi_value HelloWorld(napi_env env, napi_callback_info /*info*/) {
  napi_value result;
  napi_create_int32(env, core_hello_world(), &result);
  return result;
}

napi_value HelloWorldString(napi_env env, napi_callback_info /*info*/) {
  const char* text = core_hello_world_string();
  napi_value result;
  napi_create_string_utf8(env, text != nullptr ? text : "", NAPI_AUTO_LENGTH, &result);
  return result;
}

napi_value GlViewerCreate(napi_env env, napi_callback_info /*info*/) {
  CoreGlViewer* viewer = core_gl_viewer_create();
  napi_value result;
  napi_create_bigint_uint64(env, reinterpret_cast<uint64_t>(viewer), &result);
  return result;
}

napi_value GlViewerDestroy(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  uint64_t handle = 0;
  bool lossless = false;
  if (argc >= 1) napi_get_value_bigint_uint64(env, args[0], &handle, &lossless);
  core_gl_viewer_destroy(reinterpret_cast<CoreGlViewer*>(handle));
  return nullptr;
}

napi_value GlViewerLoad(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  uint64_t handle = 0;
  bool lossless = false;
  if (argc >= 1) napi_get_value_bigint_uint64(env, args[0], &handle, &lossless);
  char path[1024] = {0};
  size_t pathLen = 0;
  if (argc >= 2) {
    napi_get_value_string_utf8(env, args[1], path, sizeof(path) - 1, &pathLen);
  }
  int rc = -1;
  if (handle && pathLen > 0) {
    rc = core_gl_viewer_load(reinterpret_cast<CoreGlViewer*>(handle), path);
  }
  napi_value result;
  napi_create_int32(env, rc, &result);
  return result;
}

napi_value GlViewerResize(napi_env env, napi_callback_info info) {
  size_t argc = 3;
  napi_value args[3];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  uint64_t handle = 0;
  bool lossless = false;
  int32_t w = 0, h = 0;
  if (argc >= 1) napi_get_value_bigint_uint64(env, args[0], &handle, &lossless);
  if (argc >= 2) napi_get_value_int32(env, args[1], &w);
  if (argc >= 3) napi_get_value_int32(env, args[2], &h);
  if (handle) core_gl_viewer_resize(reinterpret_cast<CoreGlViewer*>(handle), w, h);
  return nullptr;
}

napi_value GlViewerRender(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  uint64_t handle = 0;
  bool lossless = false;
  if (argc >= 1) napi_get_value_bigint_uint64(env, args[0], &handle, &lossless);
  if (handle) core_gl_viewer_render(reinterpret_cast<CoreGlViewer*>(handle));
  return nullptr;
}

napi_value GlViewerOrbit(napi_env env, napi_callback_info info) {
  size_t argc = 3;
  napi_value args[3];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  uint64_t handle = 0;
  bool lossless = false;
  double dx = 0, dy = 0;
  if (argc >= 1) napi_get_value_bigint_uint64(env, args[0], &handle, &lossless);
  if (argc >= 2) napi_get_value_double(env, args[1], &dx);
  if (argc >= 3) napi_get_value_double(env, args[2], &dy);
  if (handle) core_gl_viewer_orbit(reinterpret_cast<CoreGlViewer*>(handle),
                                   static_cast<float>(dx), static_cast<float>(dy));
  return nullptr;
}

napi_value GlViewerOrbitEnd(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  uint64_t handle = 0;
  bool lossless = false;
  if (argc >= 1) napi_get_value_bigint_uint64(env, args[0], &handle, &lossless);
  if (handle) core_gl_viewer_orbit_end(reinterpret_cast<CoreGlViewer*>(handle));
  return nullptr;
}

napi_value GlViewerSetOrbitFeel(napi_env env, napi_callback_info info) {
  size_t argc = 3;
  napi_value args[3];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  uint64_t handle = 0;
  bool lossless = false;
  double sensitivity = 1.0;
  double brakeFactor = 0.10;
  if (argc >= 1) napi_get_value_bigint_uint64(env, args[0], &handle, &lossless);
  if (argc >= 2) napi_get_value_double(env, args[1], &sensitivity);
  if (argc >= 3) napi_get_value_double(env, args[2], &brakeFactor);
  if (handle) {
    core_gl_viewer_set_orbit_feel(reinterpret_cast<CoreGlViewer*>(handle),
                                  static_cast<float>(sensitivity),
                                  static_cast<float>(brakeFactor));
  }
  return nullptr;
}

napi_value GlViewerZoom(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  uint64_t handle = 0;
  bool lossless = false;
  double scale = 1.0;
  if (argc >= 1) napi_get_value_bigint_uint64(env, args[0], &handle, &lossless);
  if (argc >= 2) napi_get_value_double(env, args[1], &scale);
  if (handle) core_gl_viewer_zoom(reinterpret_cast<CoreGlViewer*>(handle),
                                  static_cast<float>(scale));
  return nullptr;
}

napi_value GlViewerDolly(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  uint64_t handle = 0;
  bool lossless = false;
  double amount = 0.0;
  if (argc >= 1) napi_get_value_bigint_uint64(env, args[0], &handle, &lossless);
  if (argc >= 2) napi_get_value_double(env, args[1], &amount);
  if (handle) core_gl_viewer_dolly(reinterpret_cast<CoreGlViewer*>(handle),
                                   static_cast<float>(amount));
  return nullptr;
}

napi_value GlViewerLightMoveX(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  uint64_t handle = 0;
  bool lossless = false;
  double amount = 0.0;
  if (argc >= 1) napi_get_value_bigint_uint64(env, args[0], &handle, &lossless);
  if (argc >= 2) napi_get_value_double(env, args[1], &amount);
  if (handle) core_gl_viewer_light_move_x(reinterpret_cast<CoreGlViewer*>(handle),
                                          static_cast<float>(amount));
  return nullptr;
}

/** Remember model path for the XComponent surface (EGL path). */
napi_value GlSetModelPath(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  char path[1024] = {0};
  size_t pathLen = 0;
  if (argc >= 1) {
    napi_get_value_string_utf8(env, args[0], path, sizeof(path) - 1, &pathLen);
  }
  g_modelPath.assign(path, pathLen);
  // Path load can resolve mtllib/textures; drop memory bytes so LoadStoredModel prefers path.
  g_modelBytes.clear();
  core_gl_reload_model_on_surfaces();
  return nullptr;
}

/**
 * Prefer for HarmonyOS: pass rawfile bytes so Assimp never depends on
 * sandboxed `/data/storage/...` paths (often unreadable via native fopen).
 */
napi_value GlSetModelData(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  if (argc < 1) return nullptr;

  void* data = nullptr;
  size_t byteLength = 0;
  if (napi_get_arraybuffer_info(env, args[0], &data, &byteLength) != napi_ok || !data ||
      byteLength == 0) {
    // Also accept Uint8Array / TypedArray
    bool isTyped = false;
    if (napi_is_typedarray(env, args[0], &isTyped) == napi_ok && isTyped) {
      napi_typedarray_type type;
      size_t length = 0;
      void* typedData = nullptr;
      napi_value arrayBuffer;
      size_t byteOffset = 0;
      if (napi_get_typedarray_info(env, args[0], &type, &length, &typedData, &arrayBuffer,
                                  &byteOffset) == napi_ok &&
          typedData && length > 0) {
        auto* bytes = static_cast<uint8_t*>(typedData);
        // length is element count; for Uint8Array element = 1 byte
        g_modelBytes.assign(bytes, bytes + length);
        core_gl_reload_model_on_surfaces();
      }
    }
    return nullptr;
  }
  auto* bytes = static_cast<uint8_t*>(data);
  g_modelBytes.assign(bytes, bytes + byteLength);
  core_gl_reload_model_on_surfaces();
  return nullptr;
}

}  // namespace

// Used by ohos_gl_xcomponent.cpp
extern "C" const char* core_gl_stored_model_path() {
  return g_modelPath.empty() ? nullptr : g_modelPath.c_str();
}

extern "C" const uint8_t* core_gl_stored_model_bytes(size_t* outSize) {
  if (outSize) *outSize = g_modelBytes.size();
  return g_modelBytes.empty() ? nullptr : g_modelBytes.data();
}

EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports) {
  napi_property_descriptor desc[] = {
      {"helloWorld", nullptr, HelloWorld, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"helloWorldString", nullptr, HelloWorldString, nullptr, nullptr, nullptr, napi_default,
       nullptr},
      {"glViewerCreate", nullptr, GlViewerCreate, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"glViewerDestroy", nullptr, GlViewerDestroy, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"glViewerLoad", nullptr, GlViewerLoad, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"glViewerResize", nullptr, GlViewerResize, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"glViewerRender", nullptr, GlViewerRender, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"glViewerOrbit", nullptr, GlViewerOrbit, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"glViewerOrbitEnd", nullptr, GlViewerOrbitEnd, nullptr, nullptr, nullptr, napi_default,
       nullptr},
      {"glViewerSetOrbitFeel", nullptr, GlViewerSetOrbitFeel, nullptr, nullptr, nullptr,
       napi_default, nullptr},
      {"glViewerZoom", nullptr, GlViewerZoom, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"glViewerDolly", nullptr, GlViewerDolly, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"glViewerLightMoveX", nullptr, GlViewerLightMoveX, nullptr, nullptr, nullptr, napi_default,
       nullptr},
      {"glSetModelPath", nullptr, GlSetModelPath, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"glSetModelData", nullptr, GlSetModelData, nullptr, nullptr, nullptr, napi_default, nullptr},
      // Must be on the first Init exports: ArkTS `import 'libcore_napi.so'` happens
      // before XComponent attaches, and that binding is what glPresentAll() uses.
      {"glPresentAll", nullptr, GlPresentAll, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"glOrbitAll", nullptr, GlOrbitAll, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"glOrbitEndAll", nullptr, GlOrbitEndAll, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"glDollyAll", nullptr, GlDollyAll, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"glLightMoveXAll", nullptr, GlLightMoveXAll, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"glLightDragAll", nullptr, GlLightDragAll, nullptr, nullptr, nullptr, napi_default, nullptr},
  };
  napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);

  // Register XComponent EGL callbacks when framework attaches the native object
  // (Init is invoked again with OH_NATIVE_XCOMPONENT_OBJ on surface create).
  core_gl_register_xcomponent(env, exports);
  return exports;
}
EXTERN_C_END

static napi_module g_module = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "core_napi",
    .nm_priv = nullptr,
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterCoreNapiModule(void) {
  napi_module_register(&g_module);
}
