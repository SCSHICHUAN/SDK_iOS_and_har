//
// renderer_data.h — same role as GL-ARKit/ARKit/GL/SCRendererData
// Owns scene state and full-frame render(); calls Model::Draw for geometry.
//
// AR mapping (SDK core, no ARKit/host-video yet):
//   SCRendererData     → RendererData
//   Camera / Shader / Model / Mesh → same modules under src/gl/
//   SCRenderer (UIView) → platform Pro app (e.g. iOSpro ModelGLView)
//   C API              → include/core/gl_viewer.h (cross-language boundary)
//

#ifndef CORE_GL_RENDERER_DATA_H_
#define CORE_GL_RENDERER_DATA_H_

#include "camera.h"
#include "model.h"
#include "shader.h"

#include <string>

class RendererData {
 public:
  RendererData();
  ~RendererData();

  RendererData(const RendererData&) = delete;
  RendererData& operator=(const RendererData&) = delete;

  /** Context must be current. Builds default model shader. */
  bool init();
  void destroy();

  bool loadModel(const std::string& path);
  bool loadModelFromMemory(const void* data, size_t size, const char* hint = ".obj");
  bool hasModel() const { return model_.loaded(); }

  void resize(int width, int height);
  /** Touch-drag rotates the box only (camera + lamp stay put). */
  void onOrbit(float dx, float dy);
  /** Pinch: scale > 1 zooms in. */
  void onPinch(float scale);
  /** Dolly along view: amount > 0 moves camera forward (closer). */
  void onDolly(float amount);
  /** Move point light on X: amount > 0 → right. */
  void onLightMoveX(float amount);
  /** Drag point light in XY (screen-style deltas from Pro). */
  void onLightDrag(float dx, float dy);
  /** One frame into the currently bound framebuffer. */
  void render();

 private:
  void applyLightingUniforms(const glm::vec3& lightPos, const glm::vec3& lampColor);
  void initLightCube();
  void destroyLightCube();
  void drawLightCube(const glm::mat4& projection, const glm::mat4& view,
                     const glm::vec3& lightPos, const glm::vec3& lampColor);

  Model model_;
  Shader shader_;
  Shader lampShader_;
  Camera camera_;
  unsigned int lightCubeVAO_ = 0;
  unsigned int lightCubeVBO_ = 0;
  int width_ = 1;
  int height_ = 1;
  /** Fixed camera orbit angles (not driven by touch). */
  float camYaw_ = 30.0f;
  float camPitch_ = 20.0f;
  /** Touch rotates the box in world space. */
  float modelYaw_ = 0.0f;
  float modelPitch_ = 0.0f;
  float distance_ = 3.0f;
  float lightX_ = -0.4f;
  float lightY_ = 1.15f;
  float lightZ_ = 0.0f;
  float elapsed_ = 0.0f;
  bool ready_ = false;
};

#endif /* CORE_GL_RENDERER_DATA_H_ */
