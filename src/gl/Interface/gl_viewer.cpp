//
// gl_viewer.cpp — C ABI 编排
//
//   Model          — load 顶点/动画 → Mesh + Animation
//   RendererData   — 纯场景数据袋
//   ViewportInput  — 手势改 Data 字段
//   Animator       — 播放（本句柄持有）
//   Render         — 整帧 Draw
//

#include "core/gl_viewer.h"

#include "animator.h"
#include "render.h"
#include "renderer_data.h"
#include "viewport_input.h"

#include <memory>
#include <string>

#if defined(__ANDROID__)
#include <android/log.h>
#define CORE_GL_LOG(...) __android_log_print(ANDROID_LOG_ERROR, "core_gl", __VA_ARGS__)
#else
#define CORE_GL_LOG(...)
#endif

struct CoreGlViewer {
  RendererData data;
  std::unique_ptr<Animator> animator;
};

static void resetAnimator(CoreGlViewer* viewer) {
  viewer->animator.reset();
  if (viewer->data.model.getAnimationCount() <= 0) return;
  viewer->animator = std::make_unique<Animator>(viewer->data.model.getAnimation(0));
  // 车辆：停在绑定姿势（避免门盖循环开合导致轮子/车身错位观感）；人模/狼：自动循环
  if (viewer->data.model.isVehicleAsset()) {
    viewer->animator->setLooping(false);
    viewer->animator->setPlaying(false);
  } else {
    viewer->animator->setLooping(true);
    viewer->animator->setPlaying(true);
  }
}

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
  viewer->animator.reset();
  viewer->data.destroy();
  delete viewer;
}

extern "C" int core_gl_viewer_load(CoreGlViewer* viewer, const char* path) {
  if (!viewer || !path) return -1;
  const bool ok = viewer->data.model.loadModel(path);
  if (ok) {
    ViewportInput::onModelLoaded(viewer->data);
    resetAnimator(viewer);
  }
  CORE_GL_LOG("gl_viewer_load: path=%s ok=%d hasModel=%d anims=%d", path, ok ? 1 : 0,
              viewer->data.hasModel() ? 1 : 0, viewer->data.model.getAnimationCount());
  return ok ? 0 : -1;
}

extern "C" int core_gl_viewer_load_memory(CoreGlViewer* viewer, const void* data, int size,
                                          const char* hint) {
  if (!viewer || !data || size <= 0) return -1;
  const bool ok =
      viewer->data.model.loadModelFromMemory(data, static_cast<size_t>(size),
                                             hint ? hint : ".obj");
  if (ok) {
    ViewportInput::onModelLoaded(viewer->data);
    resetAnimator(viewer);
  }
  return ok ? 0 : -1;
}

extern "C" void core_gl_viewer_resize(CoreGlViewer* viewer, int width, int height) {
  if (!viewer) return;
  ViewportInput::resize(viewer->data, width, height);
}

extern "C" void core_gl_viewer_orbit(CoreGlViewer* viewer, float dx, float dy) {
  if (!viewer) return;
  ViewportInput::orbit(viewer->data, dx, dy);
}

extern "C" void core_gl_viewer_orbit_end(CoreGlViewer* viewer) {
  if (!viewer) return;
  ViewportInput::orbitEnd(viewer->data);
}

extern "C" void core_gl_viewer_set_orbit_feel(CoreGlViewer* viewer, float sensitivity,
                                              float brakeFactor) {
  if (!viewer) return;
  ViewportInput::setOrbitFeel(viewer->data, sensitivity, brakeFactor);
}

extern "C" void core_gl_viewer_zoom(CoreGlViewer* viewer, float scale) {
  if (!viewer) return;
  ViewportInput::pinch(viewer->data, scale);
}

extern "C" void core_gl_viewer_dolly(CoreGlViewer* viewer, float amount) {
  if (!viewer) return;
  ViewportInput::dolly(viewer->data, amount);
}

extern "C" void core_gl_viewer_light_move_x(CoreGlViewer* viewer, float amount) {
  if (!viewer) return;
  ViewportInput::lightMoveX(viewer->data, amount);
}

extern "C" void core_gl_viewer_light_move_z(CoreGlViewer* viewer, float amount) {
  if (!viewer) return;
  ViewportInput::lightMoveZ(viewer->data, amount);
}

extern "C" void core_gl_viewer_light_drag(CoreGlViewer* viewer, float dx, float dy) {
  if (!viewer) return;
  ViewportInput::lightDrag(viewer->data, dx, dy);
}

extern "C" void core_gl_viewer_set_dir_light(CoreGlViewer* viewer, int enabled) {
  if (!viewer) return;
  ViewportInput::setDirLightEnabled(viewer->data, enabled != 0);
}

extern "C" void core_gl_viewer_set_spot_light(CoreGlViewer* viewer, int enabled) {
  if (!viewer) return;
  ViewportInput::setSpotLightEnabled(viewer->data, enabled != 0);
}

extern "C" void core_gl_viewer_set_point_light(CoreGlViewer* viewer, int enabled) {
  if (!viewer) return;
  ViewportInput::setPointLightEnabled(viewer->data, enabled != 0);
}

extern "C" void core_gl_viewer_set_point_light_color(CoreGlViewer* viewer, float r, float g,
                                                      float b) {
  if (!viewer) return;
  ViewportInput::setPointLightColor(viewer->data, r, g, b);
}

extern "C" void core_gl_viewer_camera_pan(CoreGlViewer* viewer, float amount) {
  if (!viewer) return;
  ViewportInput::cameraPan(viewer->data, amount);
}

extern "C" void core_gl_viewer_render(CoreGlViewer* viewer) {
  if (!viewer) return;
  const std::map<int, glm::mat4>* boneMats = nullptr;
  if (viewer->animator) {
    viewer->animator->updateAnimation(1.0f / 60.0f);
    boneMats = &viewer->animator->getFinalBoneMatrices();
  }
  Render::frame(viewer->data, boneMats);
}

extern "C" int core_gl_viewer_animation_count(CoreGlViewer* viewer) {
  if (!viewer) return 0;
  return viewer->data.model.getAnimationCount();
}

extern "C" const char* core_gl_viewer_animation_name(CoreGlViewer* viewer, int index) {
  if (!viewer) return nullptr;
  Animation* clip = viewer->data.model.getAnimation(index);
  if (!clip) return nullptr;
  const std::string& name = clip->getSourceName();
  return name.empty() ? nullptr : name.c_str();
}

extern "C" int core_gl_viewer_play_animation(CoreGlViewer* viewer, int index, int resetTime) {
  if (!viewer) return -1;
  Animation* clip = viewer->data.model.getAnimation(index);
  if (!clip) return -1;
  if (!viewer->animator) {
    viewer->animator = std::make_unique<Animator>(clip);
    viewer->animator->setLooping(true);
  } else {
    viewer->animator->playAnimation(clip, resetTime != 0);
  }
  viewer->animator->setPlaying(true);
  return 0;
}
