//
// gl_viewer.cpp — C ABI 实现：转发给 RendererData
// 完整「功能 / 参数 / 输入 / 返回」见 include/core/gl_viewer.h
//

#include "core/gl_viewer.h"

#include "renderer_data.h"

#if defined(__ANDROID__)
#include <android/log.h>
#define CORE_GL_LOG(...) __android_log_print(ANDROID_LOG_ERROR, "core_gl", __VA_ARGS__)
#else
#define CORE_GL_LOG(...)
#endif

/** C 句柄：包一层 RendererData */
struct CoreGlViewer {
  RendererData data;
};

/**
 * 功能：创建实例
 * 参数：无
 * 输入：须已 makeCurrent GLES3
 * 返回：句柄 / NULL
 * 转发：RendererData::init
 */
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

/**
 * 功能：销毁实例
 * 参数：viewer — 可为 NULL
 * 输入：viewer
 * 返回：无
 * 转发：RendererData::destroy + delete
 */
extern "C" void core_gl_viewer_destroy(CoreGlViewer* viewer) {
  if (!viewer) return;
  viewer->data.destroy();
  delete viewer;
}

/**
 * 功能：按路径加载模型
 * 参数：viewer, path — 均不可为 NULL
 * 输入：viewer, path
 * 返回：0 成功；-1 失败
 * 转发：RendererData::loadModel
 */
extern "C" int core_gl_viewer_load(CoreGlViewer* viewer, const char* path) {
  if (!viewer || !path) return -1;
  const bool ok = viewer->data.loadModel(path);
  CORE_GL_LOG("gl_viewer_load: path=%s ok=%d hasModel=%d", path, ok ? 1 : 0,
              viewer->data.hasModel() ? 1 : 0);
  return ok ? 0 : -1;
}

/**
 * 功能：按内存加载模型
 * 参数：viewer, data, size(>0), hint(NULL→".obj")
 * 输入：viewer, data[0..size), hint
 * 返回：0 成功；-1 失败
 * 转发：RendererData::loadModelFromMemory
 */
extern "C" int core_gl_viewer_load_memory(CoreGlViewer* viewer, const void* data, int size,
                                          const char* hint) {
  if (!viewer || !data || size <= 0) return -1;
  return viewer->data.loadModelFromMemory(data, static_cast<size_t>(size),
                                          hint ? hint : ".obj")
             ? 0
             : -1;
}

/**
 * 功能：更新视口与 arcball 宽高
 * 参数：viewer, width, height（像素，内部 ≥1）
 * 输入：viewer, width, height
 * 返回：无
 * 转发：RendererData::resize
 */
extern "C" void core_gl_viewer_resize(CoreGlViewer* viewer, int width, int height) {
  if (!viewer) return;
  viewer->data.resize(width, height);
}

/**
 * 功能：触摸拖动入队
 * 参数：viewer, dx(右+), dy(屏下+)
 * 输入：viewer, dx, dy
 * 返回：无
 * 转发：RendererData::onOrbit → inertia.move
 */
extern "C" void core_gl_viewer_orbit(CoreGlViewer* viewer, float dx, float dy) {
  if (!viewer) return;
  viewer->data.onOrbit(dx, dy);
}

/**
 * 功能：触摸抬起，启动松手滑行
 * 参数：viewer
 * 输入：viewer
 * 返回：无
 * 转发：RendererData::onOrbitEnd → inertia.end + arcball.endDrag
 */
extern "C" void core_gl_viewer_orbit_end(CoreGlViewer* viewer) {
  if (!viewer) return;
  viewer->data.onOrbitEnd();
}

/**
 * 功能：设置拖动灵敏度与松手刹车
 * 参数：viewer, sensitivity(默认1), brakeFactor(默认0.10，仅松手)
 * 输入：viewer, sensitivity, brakeFactor
 * 返回：无
 * 转发：RendererData::setOrbitFeel
 */
extern "C" void core_gl_viewer_set_orbit_feel(CoreGlViewer* viewer, float sensitivity,
                                              float brakeFactor) {
  if (!viewer) return;
  viewer->data.setOrbitFeel(sensitivity, brakeFactor);
}

/**
 * 功能：捏合缩放距离
 * 参数：viewer, scale(>1拉近；(0,1)推远；≤0忽略)
 * 输入：viewer, scale
 * 返回：无
 * 转发：RendererData::onPinch
 */
extern "C" void core_gl_viewer_zoom(CoreGlViewer* viewer, float scale) {
  if (!viewer) return;
  viewer->data.onPinch(scale);
}

/**
 * 功能：沿视线推拉相机
 * 参数：viewer, amount(>0靠近；<0远离)
 * 输入：viewer, amount
 * 返回：无
 * 转发：RendererData::onDolly
 */
extern "C" void core_gl_viewer_dolly(CoreGlViewer* viewer, float amount) {
  if (!viewer) return;
  viewer->data.onDolly(amount);
}

/**
 * 功能：灯光沿世界 X 平移
 * 参数：viewer, amount(>0右；<0左)
 * 输入：viewer, amount
 * 返回：无
 * 转发：RendererData::onLightMoveX
 */
extern "C" void core_gl_viewer_light_move_x(CoreGlViewer* viewer, float amount) {
  if (!viewer) return;
  viewer->data.onLightMoveX(amount);
}

/**
 * 功能：屏幕 XY 拖灯光
 * 参数：viewer, dx, dy（同 orbit 坐标系）
 * 输入：viewer, dx, dy
 * 返回：无
 * 转发：RendererData::onLightDrag
 */
extern "C" void core_gl_viewer_light_drag(CoreGlViewer* viewer, float dx, float dy) {
  if (!viewer) return;
  viewer->data.onLightDrag(dx, dy);
}

/**
 * 功能：画一帧
 * 参数：viewer
 * 输入：viewer；当前 FBO / 上下文已就绪
 * 返回：无
 * 转发：RendererData::render
 */
extern "C" void core_gl_viewer_render(CoreGlViewer* viewer) {
  if (!viewer) return;
  viewer->data.render();
}
