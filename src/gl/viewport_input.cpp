//
// created by stan 2026-11-18
// viewport_input.cpp — 改 RendererData 字段（浏览 / 灯光 / 视口）
//

#include "viewport_input.h"

#include "gl_platform.h"

#include <algorithm>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace ViewportInput {

namespace {

/** 按 FOV+宽高比算「整模进画」相机距离（包围球），留边 padding。 */
float fitDistance(const RendererData& data, float worldRadius, float padding = 1.4f) {
  const float R = std::max(worldRadius, 1e-3f);
  int w = data.width;
  int h = data.height;
  // load 常早于 layout：宽高未就绪时按竖屏保守估计，避免结束裁切
  if (w <= 1 || h <= 1) {
    w = 9;
    h = 19;
  }
  const float aspect = static_cast<float>(w) / static_cast<float>(h);
  const float halfFovY = glm::radians(data.camera.Zoom) * 0.5f;
  const float halfFovX = std::atan(std::tan(halfFovY) * aspect);
  const float dY = R / std::max(std::sin(halfFovY), 1e-4f);
  const float dX = R / std::max(std::sin(halfFovX), 1e-4f);
  return std::max(dX, dY) * padding;
}

float worldRadiusOf(const RendererData& data) {
  return data.model.radius() * data.model.preferredWorldScale();
}

}  // namespace

void onModelLoaded(RendererData& data) {
  // FBX（狼）：必须用 AR 的 0.018 + cameraZ=4.8；包围球半径在 FBX 上不可靠，fit 会把狼放到巨大。
  // 立方体 / SU7：按 FOV+宽高比拟合，padding 略紧 → 结束更大且仍整模进画。
  const float endScale = data.model.preferredWorldScale();
  const float endDistance =
      data.model.isFbxAsset() ? 4.8f : fitDistance(data, worldRadiusOf(data), /*padding=*/1.12f);
  data.arcball.reset();
  data.inertia.reset();

  // DebutSequence：按资源「正面」校正 + 结束 45° 斜视（逆时针转 135°）
  data.distance = endDistance;
  data.camYaw = 0.0f;
  data.camPitch = 16.0f;
  data.camPan = 0.0f;
  const float startScale = endScale * DebutSequence::kStartScaleRatio;
  data.modelScale = startScale;
  const float front = data.model.preferredFrontYawOffset();
  const float yawEnd = front + DebutSequence::kPresentYawRad;
  const float yawStart = yawEnd - DebutSequence::kSpinRad;
  data.debut.start(startScale, endScale, endDistance, yawStart, yawEnd,
                   DebutSequence::kDurationSec);

  // 爆炸 / 点选：按当前 mesh 顶点缓存零件盒与方向
  data.explode.prepare(data.model);
  data.pick.prepare(data.model);
  data.pick.clearSelection();
}

void resize(RendererData& data, int width, int height) {
  data.width = std::max(width, 1);
  data.height = std::max(height, 1);
  data.arcball.setViewport(data.width, data.height);
  glViewport(0, 0, data.width, data.height);
  // FBX 固定 4.8，不参与 fit；其它资源 layout 后按真实宽高校正
  if (!data.model.loaded() || data.model.isFbxAsset()) return;
  const float fit = fitDistance(data, worldRadiusOf(data), 1.12f);
  if (data.debut.active()) {
    data.debut.setDistance(fit);
    data.distance = fit;
  } else if (data.distance + 1e-3f < fit) {
    data.distance = fit;
  }
}

void orbit(RendererData& data, float dx, float dy) {
  if (data.debut.blocksInput()) return;  // 进场不可打断
  data.inertia.move(dx, dy);
}

void orbitEnd(RendererData& data) {
  data.inertia.end();
  data.arcball.endDrag();
}

void setOrbitFeel(RendererData& data, float sensitivity, float brakeFactor) {
  if (sensitivity > 1e-3f) data.arcball.setSensitivity(sensitivity);
  if (brakeFactor > 0.0f && brakeFactor < 1.0f) data.inertia.setBrake(brakeFactor);
}

void pinch(RendererData& data, float scale) {
  if (scale <= 0.0f) return;
  if (data.debut.blocksInput()) return;
  data.distance /= scale;
  const float minD = data.model.isFbxAsset() ? 1.0f : std::max(data.model.radius() * 0.5f, 0.5f);
  const float maxD = data.model.isFbxAsset() ? 20.0f : std::max(data.model.radius() * 20.0f, 20.0f);
  if (data.distance < minD) data.distance = minD;
  if (data.distance > maxD) data.distance = maxD;
}

void dolly(RendererData& data, float amount) {
  if (data.debut.blocksInput()) return;
  const float step = data.model.isFbxAsset() ? 0.15f : data.model.radius() * 0.15f;
  data.distance -= amount * step;
  const float minD = data.model.isFbxAsset() ? 1.0f : std::max(data.model.radius() * 0.5f, 0.5f);
  const float maxD = data.model.isFbxAsset() ? 20.0f : std::max(data.model.radius() * 20.0f, 20.0f);
  if (data.distance < minD) data.distance = minD;
  if (data.distance > maxD) data.distance = maxD;
}

namespace {

/** 灯可移动半宽：至少 ±12，大模型按半径放宽。 */
float lightTravelLimit(const RendererData& data) {
  const float R = worldRadiusOf(data);
  return std::max(R * 6.0f, 12.0f);
}

float lightStep(const RendererData& data) {
  return std::max(lightTravelLimit(data) * 0.02f, 0.12f);
}

void clampLight(RendererData& data) {
  const float lim = lightTravelLimit(data);
  data.lightX = std::clamp(data.lightX, -lim, lim);
  data.lightZ = std::clamp(data.lightZ, -lim, lim);
  data.lightY = std::clamp(data.lightY, 0.05f, lim);
}

}  // namespace

void lightMoveX(RendererData& data, float amount) {
  data.lightX += amount * lightStep(data);
  clampLight(data);
}

void lightMoveZ(RendererData& data, float amount) {
  data.lightZ += amount * lightStep(data);
  clampLight(data);
}

void lightDrag(RendererData& data, float dx, float dy) {
  const float sens = lightTravelLimit(data) * 0.0015f;
  data.lightX += dx * sens;
  data.lightY += dy * sens;
  clampLight(data);
}

void setDirLightEnabled(RendererData& data, bool enabled) {
  data.dirLightEnabled = enabled;
}

void setSpotLightEnabled(RendererData& data, bool enabled) {
  data.spotLightEnabled = enabled;
}

void setPointLightEnabled(RendererData& data, bool enabled) {
  data.pointLightEnabled = enabled;
}

void setPointLightColor(RendererData& data, float r, float g, float b) {
  data.pointLightColor = glm::vec3(std::clamp(r, 0.0f, 1.0f), std::clamp(g, 0.0f, 1.0f),
                                   std::clamp(b, 0.0f, 1.0f));
}

void cameraPan(RendererData& data, float amount) {
  if (data.debut.blocksInput()) return;
  const float lim = std::max(data.distance * 0.85f, 2.0f);
  const float step = std::max(data.distance * 0.04f, 0.08f);
  data.camPan += amount * step;
  data.camPan = std::clamp(data.camPan, -lim, lim);
}

}  // namespace ViewportInput
