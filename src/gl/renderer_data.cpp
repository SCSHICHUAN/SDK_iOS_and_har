#include "renderer_data.h"

#include "embedded_shaders.h"
#include "gl_platform.h"

#include <algorithm>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

RendererData::RendererData() = default;

RendererData::~RendererData() { destroy(); }

bool RendererData::init() {
  destroy();
  // Embedded from lighting-* (AR colors → lighting path).
  shader_ = Shader::FromSource(kLightingVertShader, kLightingFragShader);
  if (!shader_.ID) return false;
  lampShader_ = Shader::FromSource(kLampVertShader, kLampFragShader);
  if (!lampShader_.ID) return false;
  initLightCube();
  glEnable(GL_DEPTH_TEST);
  ready_ = true;
  return true;
}

void RendererData::destroy() {
  destroyLightCube();
  model_.destroy();
  lampShader_.destroy();
  shader_.destroy();
  ready_ = false;
}

bool RendererData::loadModel(const std::string& path) {
  if (!model_.loadModel(path)) return false;
  distance_ = std::max(model_.radius() * 5.5f, 2.0f);
  return true;
}

bool RendererData::loadModelFromMemory(const void* data, size_t size, const char* hint) {
  if (!model_.loadModelFromMemory(data, size, hint)) return false;
  distance_ = std::max(model_.radius() * 5.5f, 2.0f);
  return true;
}

void RendererData::resize(int width, int height) {
  width_ = std::max(width, 1);
  height_ = std::max(height, 1);
  glViewport(0, 0, width_, height_);
}

void RendererData::onOrbit(float dx, float dy) {
  // Rotate the box only — camera and lamp stay fixed in world/screen.
  modelYaw_ += dx * 0.3f;
  modelPitch_ += dy * 0.3f;
  if (modelPitch_ > 89.0f) modelPitch_ = 89.0f;
  if (modelPitch_ < -89.0f) modelPitch_ = -89.0f;
}

void RendererData::onPinch(float scale) {
  if (scale <= 0.0f) return;
  distance_ /= scale;
  float minD = model_.radius() * 0.5f;
  float maxD = model_.radius() * 20.0f;
  if (distance_ < minD) distance_ = minD;
  if (distance_ > maxD) distance_ = maxD;
}

void RendererData::onDolly(float amount) {
  const float step = model_.radius() * 0.15f;
  distance_ -= amount * step;
  float minD = model_.radius() * 0.5f;
  float maxD = model_.radius() * 20.0f;
  if (distance_ < minD) distance_ = minD;
  if (distance_ > maxD) distance_ = maxD;
}

void RendererData::onLightMoveX(float amount) {
  lightX_ += amount * 0.08f;
  if (lightX_ < -3.0f) lightX_ = -3.0f;
  if (lightX_ > 3.0f) lightX_ = 3.0f;
}

void RendererData::onLightDrag(float dx, float dy) {
  lightX_ += dx * 0.01f;
  lightY_ += dy * 0.01f;
  if (lightX_ < -3.0f) lightX_ = -3.0f;
  if (lightX_ > 3.0f) lightX_ = 3.0f;
  if (lightY_ < 0.2f) lightY_ = 0.2f;
  if (lightY_ > 3.0f) lightY_ = 3.0f;
}

void RendererData::initLightCube() {
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
  glGenVertexArrays(1, &lightCubeVAO_);
  glGenBuffers(1, &lightCubeVBO_);
  glBindVertexArray(lightCubeVAO_);
  glBindBuffer(GL_ARRAY_BUFFER, lightCubeVBO_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), reinterpret_cast<void*>(0));
  glBindVertexArray(0);
}

void RendererData::destroyLightCube() {
  if (lightCubeVAO_) glDeleteVertexArrays(1, &lightCubeVAO_);
  if (lightCubeVBO_) glDeleteBuffers(1, &lightCubeVBO_);
  lightCubeVAO_ = lightCubeVBO_ = 0;
}

void RendererData::drawLightCube(const glm::mat4& projection, const glm::mat4& view,
                                 const glm::vec3& lightPos, const glm::vec3& lampColor) {
  if (!lampShader_.ID || !lightCubeVAO_) return;
  lampShader_.use();
  lampShader_.setVec3("lampColor", lampColor);
  lampShader_.setMat4("projection", projection);
  lampShader_.setMat4("view", view);
  glm::mat4 lightModel(1.0f);
  lightModel = glm::translate(lightModel, lightPos);
  lightModel = glm::scale(lightModel, glm::vec3(0.05f));
  lampShader_.setMat4("model", lightModel);
  glBindVertexArray(lightCubeVAO_);
  glDrawArrays(GL_TRIANGLES, 0, 36);
  glBindVertexArray(0);
}

void RendererData::applyLightingUniforms(const glm::vec3& lightPos, const glm::vec3& lampColor) {
  shader_.setVec3("viewPos", camera_.Position);
  shader_.setFloat("material.shininess", 32.0f);

  shader_.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
  shader_.setVec3("dirLight.ambient", 0.35f, 0.35f, 0.35f);
  shader_.setVec3("dirLight.diffuse", 0.9f, 0.9f, 0.9f);
  shader_.setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);

  for (int i = 0; i < 3; ++i) {
    const std::string base = "pointLights[" + std::to_string(i) + "]";
    shader_.setVec3(base + ".position", 0.0f, 0.0f, 0.0f);
    shader_.setVec3(base + ".ambient", 0.0f, 0.0f, 0.0f);
    shader_.setVec3(base + ".diffuse", 0.0f, 0.0f, 0.0f);
    shader_.setVec3(base + ".specular", 0.0f, 0.0f, 0.0f);
    shader_.setFloat(base + ".constant", 1.0f);
    shader_.setFloat(base + ".linear", 0.0f);
    shader_.setFloat(base + ".quadratic", 0.0f);
  }

  // Lamp marker color drives point light — linked like AR.
  const glm::vec3 diffuseColor = lampColor * 0.8f;
  const glm::vec3 ambientColor = diffuseColor * 0.05f;
  shader_.setVec3("pointLights[3].position", lightPos);
  shader_.setVec3("pointLights[3].ambient", ambientColor);
  shader_.setVec3("pointLights[3].diffuse", diffuseColor);
  shader_.setVec3("pointLights[3].specular", 1.0f, 1.0f, 1.0f);
  shader_.setFloat("pointLights[3].constant", 1.0f);
  shader_.setFloat("pointLights[3].linear", 0.09f);
  shader_.setFloat("pointLights[3].quadratic", 0.032f);

  shader_.setVec3("spotLight.position", camera_.Position);
  shader_.setVec3("spotLight.direction", camera_.Front);
  shader_.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
  shader_.setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
  shader_.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
  shader_.setFloat("spotLight.constant", 1.0f);
  shader_.setFloat("spotLight.linear", 0.09f);
  shader_.setFloat("spotLight.quadratic", 0.032f);
  shader_.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
  shader_.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));
}

void RendererData::render() {
  if (!ready_) return;

  glClearColor(0.12f, 0.13f, 0.16f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  if (!model_.loaded() || !shader_.ID) return;

  elapsed_ += 1.0f / 60.0f;
  const glm::vec3 center = model_.center();
  // Fixed camera; touch only spins the box.
  camera_.SetOrbit(center, distance_, camYaw_, camPitch_);

  glm::mat4 model(1.0f);
  model = glm::translate(model, center);
  model = glm::rotate(model, glm::radians(modelYaw_), glm::vec3(0.0f, 1.0f, 0.0f));
  model = glm::rotate(model, glm::radians(modelPitch_), glm::vec3(1.0f, 0.0f, 0.0f));
  model = glm::translate(model, -center);

  glm::mat4 view = camera_.GetViewMatrix();
  float aspect = static_cast<float>(width_) / static_cast<float>(height_);
  glm::mat4 projection =
      glm::perspective(glm::radians(camera_.Zoom), aspect, 0.05f, 500.0f);

  static const glm::vec3 kLampColors[] = {
      {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f},
      {1.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 1.0f},
  };
  const int lampIdx = static_cast<int>(elapsed_ * 2.0f) % 6;
  const glm::vec3 lampColor = kLampColors[lampIdx];
  const glm::vec3 lightPos(lightX_, lightY_, lightZ_);

  drawLightCube(projection, view, lightPos, lampColor);

  shader_.use();
  shader_.setMat4("projection", projection);
  shader_.setMat4("view", view);
  shader_.setMat4("model", model);
  applyLightingUniforms(lightPos, lampColor);

  model_.Draw(shader_);
}
