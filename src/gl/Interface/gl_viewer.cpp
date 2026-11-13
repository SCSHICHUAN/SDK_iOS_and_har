//
// gl_viewer.cpp — C ABI over RendererData (for Swift / JNI / NAPI).
// Platform UIView/Surface stays in Pro apps, like AR's SCRenderer.mm.
//

#include "core/gl_viewer.h"

#include "renderer_data.h"

#if defined(__ANDROID__)
#include <android/log.h>
#define CORE_GL_LOG(...) __android_log_print(ANDROID_LOG_ERROR, "core_gl", __VA_ARGS__)
#else
#define CORE_GL_LOG(...)
#endif

struct CoreGlViewer {
  RendererData data;
};

extern "C" CoreGlViewer* core_gl_viewer_create(void) {
  auto* v = new CoreGlViewer();
  if (!v->data.init()) {
    CORE_GL_LOG("gl_viewer_create: init failed");
    delete v;
    return nullptr;
  }
  CORE_GL_LOG("gl_viewer_create: ok");
  return v;
}

extern "C" void core_gl_viewer_destroy(CoreGlViewer* viewer) {
  if (!viewer) return;
  viewer->data.destroy();
  delete viewer;
}

extern "C" int core_gl_viewer_load(CoreGlViewer* viewer, const char* path) {
  if (!viewer || !path) return -1;
  const bool ok = viewer->data.loadModel(path);
  CORE_GL_LOG("gl_viewer_load: path=%s ok=%d hasModel=%d", path, ok ? 1 : 0,
              viewer->data.hasModel() ? 1 : 0);
  return ok ? 0 : -1;
}

extern "C" int core_gl_viewer_load_memory(CoreGlViewer* viewer, const void* data, int size,
                                          const char* hint) {
  if (!viewer || !data || size <= 0) return -1;
  return viewer->data.loadModelFromMemory(data, static_cast<size_t>(size),
                                          hint ? hint : ".obj")
             ? 0
             : -1;
}

extern "C" void core_gl_viewer_resize(CoreGlViewer* viewer, int width, int height) {
  if (!viewer) return;
  viewer->data.resize(width, height);
}

extern "C" void core_gl_viewer_orbit(CoreGlViewer* viewer, float dx, float dy) {
  if (!viewer) return;
  viewer->data.onOrbit(dx, dy);
}

extern "C" void core_gl_viewer_orbit_end(CoreGlViewer* viewer) {
  if (!viewer) return;
  viewer->data.onOrbitEnd();
}

extern "C" void core_gl_viewer_zoom(CoreGlViewer* viewer, float scale) {
  if (!viewer) return;
  viewer->data.onPinch(scale);
}

extern "C" void core_gl_viewer_dolly(CoreGlViewer* viewer, float amount) {
  if (!viewer) return;
  viewer->data.onDolly(amount);
}

extern "C" void core_gl_viewer_light_move_x(CoreGlViewer* viewer, float amount) {
  if (!viewer) return;
  viewer->data.onLightMoveX(amount);
}

extern "C" void core_gl_viewer_light_drag(CoreGlViewer* viewer, float dx, float dy) {
  if (!viewer) return;
  viewer->data.onLightDrag(dx, dy);
}

extern "C" void core_gl_viewer_render(CoreGlViewer* viewer) {
  if (!viewer) return;
  viewer->data.render();
}
