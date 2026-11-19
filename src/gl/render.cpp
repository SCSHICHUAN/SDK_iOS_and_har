//
// created by stan 2026-11-18
// render.cpp — 整帧：清屏 / 相机 / 灯 / MVP / Model::Draw
// 引入 RendererData；灯光 uniform / 灯立方体绘制也在这里（不再塞进 Data）
//

#include "render.h"

#include "gl_platform.h"

#include <algorithm>
#include <cmath>
#include <string>

#include <glm/gtc/matrix_transform.hpp>

namespace {

void applyLightingUniforms(RendererData& data, const glm::vec3& lightPos,
                           const glm::vec3& lampColor) {
  data.shader.setVec3("viewPos", data.camera.Position);
  data.shader.setFloat("material.shininess", 32.0f);

  // 太阳（平行光）：从点光源分一部分亮度过来
  data.shader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
  if (data.dirLightEnabled) {
    data.shader.setVec3("dirLight.ambient", 0.48f, 0.48f, 0.48f);
    data.shader.setVec3("dirLight.diffuse", 1.2f, 1.2f, 1.15f);
    data.shader.setVec3("dirLight.specular", 0.65f, 0.65f, 0.6f);
  } else {
    data.shader.setVec3("dirLight.ambient", 0.0f, 0.0f, 0.0f);
    data.shader.setVec3("dirLight.diffuse", 0.0f, 0.0f, 0.0f);
    data.shader.setVec3("dirLight.specular", 0.0f, 0.0f, 0.0f);
  }

  for (int i = 0; i < 3; ++i) {
    const std::string base = "pointLights[" + std::to_string(i) + "]";
    data.shader.setVec3(base + ".position", 0.0f, 0.0f, 0.0f);
    data.shader.setVec3(base + ".ambient", 0.0f, 0.0f, 0.0f);
    data.shader.setVec3(base + ".diffuse", 0.0f, 0.0f, 0.0f);
    data.shader.setVec3(base + ".specular", 0.0f, 0.0f, 0.0f);
    data.shader.setFloat(base + ".constant", 1.0f);
    data.shader.setFloat(base + ".linear", 0.0f);
    data.shader.setFloat(base + ".quadratic", 0.0f);
  }

  // 点光源「灯」：再压一档，色卡仍生效；关则清零
  if (data.pointLightEnabled) {
    const glm::vec3 diffuseColor = lampColor * 0.85f;
    const glm::vec3 ambientColor = lampColor * 0.05f;
    data.shader.setVec3("pointLights[3].position", lightPos);
    data.shader.setVec3("pointLights[3].ambient", ambientColor);
    data.shader.setVec3("pointLights[3].diffuse", diffuseColor);
    data.shader.setVec3("pointLights[3].specular", lampColor * 0.55f);
    data.shader.setFloat("pointLights[3].constant", 1.0f);
    data.shader.setFloat("pointLights[3].linear", 0.09f);
    data.shader.setFloat("pointLights[3].quadratic", 0.032f);
  } else {
    data.shader.setVec3("pointLights[3].position", lightPos);
    data.shader.setVec3("pointLights[3].ambient", 0.0f, 0.0f, 0.0f);
    data.shader.setVec3("pointLights[3].diffuse", 0.0f, 0.0f, 0.0f);
    data.shader.setVec3("pointLights[3].specular", 0.0f, 0.0f, 0.0f);
    data.shader.setFloat("pointLights[3].constant", 1.0f);
    data.shader.setFloat("pointLights[3].linear", 0.0f);
    data.shader.setFloat("pointLights[3].quadratic", 0.0f);
  }

  // 手电（聚光）：压暗一档，锥角适中
  data.shader.setVec3("spotLight.position", data.camera.Position);
  data.shader.setVec3("spotLight.direction", data.camera.Front);
  data.shader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
  if (data.spotLightEnabled) {
    data.shader.setVec3("spotLight.diffuse", 1.05f, 1.05f, 1.0f);
    data.shader.setVec3("spotLight.specular", 0.7f, 0.7f, 0.65f);
  } else {
    data.shader.setVec3("spotLight.diffuse", 0.0f, 0.0f, 0.0f);
    data.shader.setVec3("spotLight.specular", 0.0f, 0.0f, 0.0f);
  }
  data.shader.setFloat("spotLight.constant", 1.0f);
  data.shader.setFloat("spotLight.linear", 0.09f);
  data.shader.setFloat("spotLight.quadratic", 0.028f);
  data.shader.setFloat("spotLight.cutOff", glm::cos(glm::radians(15.0f)));
  data.shader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(22.0f)));
}

void drawLightCube(RendererData& data, const glm::mat4& projection, const glm::mat4& view,
                   const glm::vec3& lightPos, const glm::vec3& lampColor) {
  if (!data.lampShader.ID || !data.lightCubeVAO) return;
  data.lampShader.use();
  data.lampShader.setVec3("lampColor", lampColor);
  data.lampShader.setMat4("projection", projection);
  data.lampShader.setMat4("view", view);
  glm::mat4 lightModel(1.0f);
  lightModel = glm::translate(lightModel, lightPos);
  lightModel = glm::scale(lightModel, glm::vec3(0.05f));
  data.lampShader.setMat4("model", lightModel);
  glBindVertexArray(data.lightCubeVAO);
  glDrawArrays(GL_TRIANGLES, 0, 36);
  glBindVertexArray(0);
}

}  // namespace

void Render::frame(RendererData& data, const std::map<int, glm::mat4>* boneMatrices) {
  if (!data.ready) return;

  glClearColor(0.12f, 0.13f, 0.16f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  if (!data.model.loaded() || !data.shader.ID) return;

  const float dt = 1.0f / 60.0f;
  data.elapsed += dt;

  // DebutSequence：easeOutCubic 管收尾；进场中忽略手势/惯性
  float debutYaw = 0.0f;
  float drawScale = data.modelScale;
  if (data.debut.active()) {
    data.debut.tick(dt);
    data.distance = data.debut.distance();
    drawScale = data.debut.scale();
    debutYaw = data.debut.yawRadians();
    if (!data.debut.active()) {
      data.debut.settle(data.arcball, data.modelScale);
      drawScale = data.modelScale;
      debutYaw = 0.0f;
    }
  }

  // 模型平移到原点再缩放：大单位 FBX 与点光源/相机同一套世界尺度
  const glm::vec3 center = data.model.center();
  data.camera.SetOrbit(glm::vec3(0.0f), data.distance, data.camYaw, data.camPitch);
  // 左右按钮：沿水平 Right 平移相机与注视点（改相机位置，不是转 yaw）
  if (std::fabs(data.camPan) > 1e-5f) {
    glm::vec3 right = data.camera.Right;
    right.y = 0.0f;
    if (glm::dot(right, right) > 1e-8f) {
      right = glm::normalize(right);
      const glm::vec3 pan = right * data.camPan;
      data.camera.Position += pan;
      data.camera.LookAtPoint(pan);
    }
  }

  float idx = 0.0f, idy = 0.0f;
  if (!data.debut.active() && data.inertia.tick(idx, idy)) {
    data.arcball.drag(idx, idy, data.camera.Right, data.camera.Up);
  }

  glm::mat4 model(1.0f);
  model = data.arcball.rotationMatrix() *
          glm::rotate(glm::mat4(1.0f), debutYaw, glm::vec3(0.0f, 1.0f, 0.0f)) *
          glm::scale(glm::mat4(1.0f), glm::vec3(drawScale)) *
          glm::translate(glm::mat4(1.0f), -center);

  glm::mat4 view = data.camera.GetViewMatrix();
  const float aspect = static_cast<float>(data.width) / static_cast<float>(data.height);
  const float nearPlane = std::max(0.01f, data.distance * 0.001f);
  const float farPlane = std::max(100.0f, data.distance * 20.0f);
  glm::mat4 projection =
      glm::perspective(glm::radians(data.camera.Zoom), aspect, nearPlane, farPlane);

  const glm::vec3 lampColor = data.pointLightColor;
  const glm::vec3 lightPos(data.lightX, data.lightY, data.lightZ);

  if (data.pointLightEnabled) {
    drawLightCube(data, projection, view, lightPos, lampColor);
  }

  data.explode.tick(dt);

  data.shader.use();
  data.shader.setMat4("projection", projection);
  data.shader.setMat4("view", view);
  applyLightingUniforms(data, lightPos, lampColor);

  // 分片画：每 mesh 可带爆炸位移 + 选中高亮
  const int selected = data.pick.selected();
  const bool doExplode = data.explode.prepared() && data.explode.amount() > 1e-5f;
  for (size_t i = 0; i < data.model.meshes.size(); ++i) {
    glm::mat4 meshModel = model;
    if (doExplode) {
      meshModel = model * data.explode.offsetMatrix(static_cast<int>(i));
    }
    data.shader.setMat4("model", meshModel);
    data.shader.setFloat("selectBoost", (selected == static_cast<int>(i)) ? 1.0f : 0.0f);
    data.model.meshes[i].Draw(data.shader, boneMatrices);
  }

  data.lastModel = model;
  data.lastView = view;
  data.lastProj = projection;
}
