// XComponent + EGL surface for HarmonyOS (platform layer ≈ Android GLSurfaceView).
// MakeCurrent on the native window, then call core_gl_viewer_*.

#include <ace/xcomponent/native_interface_xcomponent.h>
#include <hilog/log.h>
#include <napi/native_api.h>
#include <native_window/external_window.h>

#include <EGL/egl.h>
#include <GLES3/gl3.h>

#include <cstdint>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>

#include "core/gl_viewer.h"

#undef LOG_DOMAIN
#undef LOG_TAG
#define LOG_DOMAIN 0x3200
#define LOG_TAG "CoreGL"

extern "C" const char* core_gl_stored_model_path();
extern "C" const uint8_t* core_gl_stored_model_bytes(size_t* outSize);

bool LoadStoredModel(CoreGlViewer* viewer) {
  size_t nbytes = 0;
  if (const uint8_t* bytes = core_gl_stored_model_bytes(&nbytes)) {
    int rc = core_gl_viewer_load_memory(viewer, bytes, static_cast<int>(nbytes), ".obj");
    OH_LOG_INFO(LOG_APP, "load memory %{public}zu bytes -> %{public}d", nbytes, rc);
    return rc == 0;
  }
  if (const char* path = core_gl_stored_model_path()) {
    int rc = core_gl_viewer_load(viewer, path);
    OH_LOG_INFO(LOG_APP, "load path %{public}s -> %{public}d", path, rc);
    return rc == 0;
  }
  OH_LOG_WARN(LOG_APP, "no model bytes/path yet");
  return false;
}

namespace {

struct SurfaceState {
  EGLDisplay display = EGL_NO_DISPLAY;
  EGLSurface surface = EGL_NO_SURFACE;
  EGLContext context = EGL_NO_CONTEXT;
  OHNativeWindow* window = nullptr;
  CoreGlViewer* viewer = nullptr;
  int width = 1;
  int height = 1;
  bool hasTouch = false;
  float lastX = 0;
  float lastY = 0;
};

std::mutex g_mu;
std::unordered_map<std::string, SurfaceState> g_surfaces;

SurfaceState* Find(OH_NativeXComponent* component) {
  char id[OH_XCOMPONENT_ID_LEN_MAX + 1] = {0};
  uint64_t len = OH_XCOMPONENT_ID_LEN_MAX;
  if (OH_NativeXComponent_GetXComponentId(component, id, &len) != OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
    return nullptr;
  }
  auto it = g_surfaces.find(id);
  return it == g_surfaces.end() ? nullptr : &it->second;
}

void ReleaseCurrent(SurfaceState& st) {
  if (st.display != EGL_NO_DISPLAY) {
    eglMakeCurrent(st.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  }
}

bool EnsureEgl(SurfaceState& st, void* window, int width, int height) {
  auto* nativeWindow = reinterpret_cast<OHNativeWindow*>(window);
  st.window = nativeWindow;
  // Required on OHOS: set producer buffer size before eglCreateWindowSurface.
  OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, SET_BUFFER_GEOMETRY, width, height);

  if (st.display != EGL_NO_DISPLAY) return true;
  st.display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  if (st.display == EGL_NO_DISPLAY) {
    OH_LOG_ERROR(LOG_APP, "eglGetDisplay failed");
    return false;
  }
  EGLint major = 0, minor = 0;
  if (!eglInitialize(st.display, &major, &minor)) {
    OH_LOG_ERROR(LOG_APP, "eglInitialize failed %{public}d", eglGetError());
    return false;
  }

  const EGLint attribs[] = {
      EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
      EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
      EGL_RED_SIZE, 8,
      EGL_GREEN_SIZE, 8,
      EGL_BLUE_SIZE, 8,
      EGL_ALPHA_SIZE, 8,
      EGL_DEPTH_SIZE, 24,
      EGL_NONE,
  };
  EGLConfig config = nullptr;
  EGLint num = 0;
  if (!eglChooseConfig(st.display, attribs, &config, 1, &num) || num < 1) {
    OH_LOG_ERROR(LOG_APP, "eglChooseConfig failed %{public}d", eglGetError());
    return false;
  }

  const EGLint ctxAttr[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
  st.context = eglCreateContext(st.display, config, EGL_NO_CONTEXT, ctxAttr);
  if (st.context == EGL_NO_CONTEXT) {
    OH_LOG_ERROR(LOG_APP, "eglCreateContext failed %{public}d", eglGetError());
    return false;
  }

  st.surface = eglCreateWindowSurface(st.display, config,
                                      reinterpret_cast<EGLNativeWindowType>(window), nullptr);
  if (st.surface == EGL_NO_SURFACE) {
    OH_LOG_ERROR(LOG_APP, "eglCreateWindowSurface failed %{public}d", eglGetError());
    return false;
  }
  if (!eglMakeCurrent(st.display, st.surface, st.surface, st.context)) {
    OH_LOG_ERROR(LOG_APP, "eglMakeCurrent failed %{public}d", eglGetError());
    return false;
  }
  return true;
}

void DestroyEgl(SurfaceState& st) {
  if (st.display != EGL_NO_DISPLAY) {
    eglMakeCurrent(st.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (st.surface != EGL_NO_SURFACE) eglDestroySurface(st.display, st.surface);
    if (st.context != EGL_NO_CONTEXT) eglDestroyContext(st.display, st.context);
    eglTerminate(st.display);
  }
  st.display = EGL_NO_DISPLAY;
  st.surface = EGL_NO_SURFACE;
  st.context = EGL_NO_CONTEXT;
  st.window = nullptr;
}

void OnSurfaceCreated(OH_NativeXComponent* component, void* window) {
  char id[OH_XCOMPONENT_ID_LEN_MAX + 1] = {0};
  uint64_t len = OH_XCOMPONENT_ID_LEN_MAX;
  if (OH_NativeXComponent_GetXComponentId(component, id, &len) != OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
    OH_LOG_ERROR(LOG_APP, "GetXComponentId failed");
    return;
  }
  std::lock_guard<std::mutex> lock(g_mu);
  SurfaceState& st = g_surfaces[id];

  uint64_t w = 0, h = 0;
  OH_NativeXComponent_GetXComponentSize(component, window, &w, &h);
  st.width = static_cast<int>(w > 0 ? w : 1);
  st.height = static_cast<int>(h > 0 ? h : 1);
  OH_LOG_INFO(LOG_APP, "OnSurfaceCreated id=%{public}s size=%{public}dx%{public}d", id, st.width,
              st.height);

  if (!EnsureEgl(st, window, st.width, st.height)) {
    g_surfaces.erase(id);
    return;
  }

  if (!st.viewer) st.viewer = core_gl_viewer_create();
  if (!st.viewer) {
    OH_LOG_ERROR(LOG_APP, "core_gl_viewer_create failed");
    DestroyEgl(st);
    g_surfaces.erase(id);
    return;
  }
  core_gl_viewer_resize(st.viewer, st.width, st.height);
  LoadStoredModel(st.viewer);
  core_gl_viewer_render(st.viewer);
  eglSwapBuffers(st.display, st.surface);
  // Release so the ArkTS timer thread can MakeCurrent in GlPresentAll.
  ReleaseCurrent(st);
}

void OnSurfaceChanged(OH_NativeXComponent* component, void* window) {
  std::lock_guard<std::mutex> lock(g_mu);
  SurfaceState* st = Find(component);
  if (!st || !st->viewer) return;
  uint64_t w = 0, h = 0;
  OH_NativeXComponent_GetXComponentSize(component, window, &w, &h);
  st->width = static_cast<int>(w > 0 ? w : 1);
  st->height = static_cast<int>(h > 0 ? h : 1);
  if (st->window) {
    OH_NativeWindow_NativeWindowHandleOpt(st->window, SET_BUFFER_GEOMETRY, st->width, st->height);
  }
  if (st->display == EGL_NO_DISPLAY) return;
  if (!eglMakeCurrent(st->display, st->surface, st->surface, st->context)) return;
  core_gl_viewer_resize(st->viewer, st->width, st->height);
  ReleaseCurrent(*st);
  OH_LOG_INFO(LOG_APP, "OnSurfaceChanged %{public}dx%{public}d", st->width, st->height);
}

void OnSurfaceDestroyed(OH_NativeXComponent* component, void* /*window*/) {
  char id[OH_XCOMPONENT_ID_LEN_MAX + 1] = {0};
  uint64_t len = OH_XCOMPONENT_ID_LEN_MAX;
  if (OH_NativeXComponent_GetXComponentId(component, id, &len) != OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
    return;
  }
  std::lock_guard<std::mutex> lock(g_mu);
  auto it = g_surfaces.find(id);
  if (it == g_surfaces.end()) return;
  OH_LOG_INFO(LOG_APP, "OnSurfaceDestroyed %{public}s", id);
  if (it->second.viewer) {
    if (it->second.display != EGL_NO_DISPLAY) {
      eglMakeCurrent(it->second.display, it->second.surface, it->second.surface,
                     it->second.context);
    }
    core_gl_viewer_destroy(it->second.viewer);
    it->second.viewer = nullptr;
  }
  DestroyEgl(it->second);
  g_surfaces.erase(it);
}

void DispatchTouchEvent(OH_NativeXComponent* component, void* window) {
  std::lock_guard<std::mutex> lock(g_mu);
  SurfaceState* st = Find(component);
  if (!st || !st->viewer) return;

  OH_NativeXComponent_TouchEvent event;
  std::memset(&event, 0, sizeof(event));
  // window must be the native window from this callback — nullptr always fails.
  if (OH_NativeXComponent_GetTouchEvent(component, window, &event) !=
      OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
    return;
  }
  if (event.numPoints < 1) return;
  const float x = event.touchPoints[0].x;
  const float y = event.touchPoints[0].y;
  if (event.type == OH_NATIVEXCOMPONENT_DOWN) {
    st->hasTouch = true;
    st->lastX = x;
    st->lastY = y;
  } else if (event.type == OH_NATIVEXCOMPONENT_MOVE && st->hasTouch) {
    // Pass raw screen deltas; Arcball flips Y internally (same as ArkTS/Android/iOS).
    core_gl_viewer_orbit(st->viewer, x - st->lastX, y - st->lastY);
    st->lastX = x;
    st->lastY = y;
  } else if (event.type == OH_NATIVEXCOMPONENT_UP || event.type == OH_NATIVEXCOMPONENT_CANCEL) {
    st->hasTouch = false;
    core_gl_viewer_orbit_end(st->viewer);
  }
}

OH_NativeXComponent_Callback g_callback = {
    .OnSurfaceCreated = OnSurfaceCreated,
    .OnSurfaceChanged = OnSurfaceChanged,
    .OnSurfaceDestroyed = OnSurfaceDestroyed,
    .DispatchTouchEvent = DispatchTouchEvent,
};

}  // namespace

extern "C" void core_gl_reload_model_on_surfaces() {
  size_t nbytes = 0;
  const uint8_t* bytes = core_gl_stored_model_bytes(&nbytes);
  const char* path = core_gl_stored_model_path();
  if (!bytes && !path) return;
  std::lock_guard<std::mutex> lock(g_mu);
  for (auto& kv : g_surfaces) {
    SurfaceState& st = kv.second;
    if (!st.viewer || st.display == EGL_NO_DISPLAY) continue;
    if (!eglMakeCurrent(st.display, st.surface, st.surface, st.context)) continue;
    LoadStoredModel(st.viewer);
  }
}

extern "C" napi_value GlPresentAll(napi_env /*env*/, napi_callback_info /*info*/) {
  std::lock_guard<std::mutex> lock(g_mu);
  for (auto& kv : g_surfaces) {
    SurfaceState& st = kv.second;
    if (!st.viewer || st.display == EGL_NO_DISPLAY) continue;
    if (!eglMakeCurrent(st.display, st.surface, st.surface, st.context)) {
      OH_LOG_ERROR(LOG_APP, "GlPresentAll MakeCurrent failed %{public}d", eglGetError());
      continue;
    }
    glViewport(0, 0, st.width, st.height);
    core_gl_viewer_render(st.viewer);
    if (!eglSwapBuffers(st.display, st.surface)) {
      OH_LOG_ERROR(LOG_APP, "eglSwapBuffers failed %{public}d", eglGetError());
    }
    // Keep context current on the present thread — releasing every frame breaks
    // OHOS DGLES compositor (createimage with ctx null).
  }
  return nullptr;
}

/** ArkTS onTouch fallback — SURFACE XComponent native touch is not always delivered. */
extern "C" napi_value GlOrbitAll(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  double dx = 0, dy = 0;
  if (argc >= 1) napi_get_value_double(env, args[0], &dx);
  if (argc >= 2) napi_get_value_double(env, args[1], &dy);
  std::lock_guard<std::mutex> lock(g_mu);
  for (auto& kv : g_surfaces) {
    SurfaceState& st = kv.second;
    if (!st.viewer) continue;
    core_gl_viewer_orbit(st.viewer, static_cast<float>(dx), static_cast<float>(dy));
  }
  return nullptr;
}

extern "C" napi_value GlOrbitEndAll(napi_env /*env*/, napi_callback_info /*info*/) {
  std::lock_guard<std::mutex> lock(g_mu);
  for (auto& kv : g_surfaces) {
    SurfaceState& st = kv.second;
    if (!st.viewer) continue;
    core_gl_viewer_orbit_end(st.viewer);
  }
  return nullptr;
}

/** ArkTS dolly buttons — amount > 0 moves camera forward. */
extern "C" napi_value GlDollyAll(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  double amount = 0;
  if (argc >= 1) napi_get_value_double(env, args[0], &amount);
  std::lock_guard<std::mutex> lock(g_mu);
  for (auto& kv : g_surfaces) {
    SurfaceState& st = kv.second;
    if (!st.viewer) continue;
    core_gl_viewer_dolly(st.viewer, static_cast<float>(amount));
  }
  return nullptr;
}

/** ArkTS point-light X move — amount > 0 moves light right. */
extern "C" napi_value GlLightMoveXAll(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  double amount = 0;
  if (argc >= 1) napi_get_value_double(env, args[0], &amount);
  std::lock_guard<std::mutex> lock(g_mu);
  for (auto& kv : g_surfaces) {
    SurfaceState& st = kv.second;
    if (!st.viewer) continue;
    core_gl_viewer_light_move_x(st.viewer, static_cast<float>(amount));
  }
  return nullptr;
}

/** ArkTS point-light XY drag (Pro toggle mode). */
extern "C" napi_value GlLightDragAll(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  double dx = 0, dy = 0;
  if (argc >= 1) napi_get_value_double(env, args[0], &dx);
  if (argc >= 2) napi_get_value_double(env, args[1], &dy);
  std::lock_guard<std::mutex> lock(g_mu);
  for (auto& kv : g_surfaces) {
    SurfaceState& st = kv.second;
    if (!st.viewer) continue;
    core_gl_viewer_light_drag(st.viewer, static_cast<float>(dx), static_cast<float>(dy));
  }
  return nullptr;
}

extern "C" void core_gl_register_xcomponent(napi_env env, napi_value exports) {
  bool hasProp = false;
  if (napi_has_named_property(env, exports, OH_NATIVE_XCOMPONENT_OBJ, &hasProp) != napi_ok ||
      !hasProp) {
    return;
  }

  napi_value exportInstance = nullptr;
  if (napi_get_named_property(env, exports, OH_NATIVE_XCOMPONENT_OBJ, &exportInstance) !=
      napi_ok) {
    return;
  }

  OH_NativeXComponent* nativeXComponent = nullptr;
  if (napi_unwrap(env, exportInstance, reinterpret_cast<void**>(&nativeXComponent)) != napi_ok ||
      nativeXComponent == nullptr) {
    return;
  }

  OH_NativeXComponent_RegisterCallback(nativeXComponent, &g_callback);
  OH_LOG_INFO(LOG_APP, "XComponent callbacks registered");
}
