//
// renderer_data.cpp — 只负责场景 GL 资源的创建/销毁
//

#include "renderer_data.h"

#include "embedded_shaders.h"
#include "gl_platform.h"

bool RendererData::init() {
  destroy();
  shader = Shader::FromSource(kLightingVertShader, kLightingFragShader);
  if (!shader.ID) return false;
  lampShader = Shader::FromSource(kLampVertShader, kLampFragShader);
  if (!lampShader.ID) return false;

  // 灯标记立方体（几何资源挂在 Data 上，Draw 在 Render）
  const float vertices[] = {
      -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,
       0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f,

      -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
       0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f,

      -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
      -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,

       0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f,
       0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,

      -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,
       0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f,

      -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,
       0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f,
  };
  glGenVertexArrays(1, &lightCubeVAO);
  glGenBuffers(1, &lightCubeVBO);
  glBindVertexArray(lightCubeVAO);
  glBindBuffer(GL_ARRAY_BUFFER, lightCubeVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), reinterpret_cast<void*>(0));
  glBindVertexArray(0);

  glEnable(GL_DEPTH_TEST);
  inertia.setBrake(kInertiaBrakeFactor);
  ready = true;
  return true;
}

void RendererData::destroy() {
  if (lightCubeVAO) glDeleteVertexArrays(1, &lightCubeVAO);
  if (lightCubeVBO) glDeleteBuffers(1, &lightCubeVBO);
  lightCubeVAO = lightCubeVBO = 0;
  model.destroy();
  lampShader.destroy();
  shader.destroy();
  ready = false;
}
