//
// arcball_mobile.h — mobile tumble: local Y + camera OX
// 移动端浏览转物体：左右绕物体局部 Y，上下绕相机 OX；可同帧同时转
//
// 幅度对齐 OrbitControls（不是挤球半径）：
//   θ = 2π · (px / Height) · Sensitivity
//   Sensitivity = 1 → 拖一屏高 ≈ 转一圈；越大同位移转得越多
//
// 符号仍借用 Arcball 球面叉乘；局部 Y 相对相机倒置时左右取反。
// Radius 只参与 mapToSphere 取符号，不进转角公式。
//

#ifndef CORE_GL_ARCBALL_MOBILE_H_
#define CORE_GL_ARCBALL_MOBILE_H_

#include <algorithm>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

/** OrbitControls rotateSpeed；1 ≈ 一屏高一圈 */
const float kArcballMobileSensitivity = 4.0f;
/** 上下相对左右的增益（略压一点俯仰） */
const float kArcballMobileVerticalGain = 0.85f;

class ArcballMobile {
 public:
  static constexpr float kTwoPi = 6.28318530717958647692f;

  // —— members ——
  int Width;              // 视口宽
  int Height;             // 视口高；θ 分母用它（OrbitControls）
  float Radius;           // 仅符号球用，不进幅度
  float Sensitivity;      // 齿轮比 / rotateSpeed
  float VerticalGain;     // 俯仰相对偏航增益
  glm::quat Orientation;  // 累计模型朝向
  glm::vec4 LastTheta;    // 最近一次 (axis.xyz, θ)

  // —— methods ——
  ArcballMobile()
      : Width(1),
        Height(1),
        Radius(200.0f),
        Sensitivity(kArcballMobileSensitivity),
        VerticalGain(kArcballMobileVerticalGain),
        Orientation(1.0f, 0.0f, 0.0f, 0.0f),
        LastTheta(0.0f) {}

  /** 视口尺寸；Radius 仍取短边一半（给符号球） */
  void setViewport(int width, int height) {
    Width = std::max(width, 1);
    Height = std::max(height, 1);
    Radius = 0.5f * static_cast<float>(std::min(Width, Height));
  }

  void setRadius(float pixels) {
    if (pixels > 1.0f) Radius = pixels;
  }

  float radius() const { return Radius; }

  /**
   * 齿轮比（不是挤球半径）。
   * 1 ≈ 一屏高一圈；越大 = 同样拖动模型转得越多。
   */
  void setSensitivity(float s) {
    if (s > 1e-3f) Sensitivity = s;
  }

  float sensitivity() const { return Sensitivity; }

  /** 上下相对左右的增益 */
  void setVerticalGain(float g) {
    if (g > 1e-3f) VerticalGain = g;
  }

  float verticalGain() const { return VerticalGain; }

  /** 重置累计朝向 */
  void reset() {
    Orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    LastTheta = glm::vec4(0.0f);
  }

  /**
   * dx → 物体局部 +Y（右乘）；dy → 相机 OX（左乘）。
   * 幅度：θ = 2π · px / Height · Sensitivity。
   * camRight / camUp 来自 Camera，用于选轴与倒置修正。
   */
  void drag(float dx, float dy, const glm::vec3& camRight, const glm::vec3& camUp) {
    if (dx == 0.0f && dy == 0.0f) return;

    const glm::vec3 camOX = safeNorm(camRight, glm::vec3(1.0f, 0.0f, 0.0f));
    const glm::vec3 camOY = safeNorm(camUp, glm::vec3(0.0f, 1.0f, 0.0f));

    // 左右：Orbit 角 + 局部 Y / 倒置符号修正
    if (dx != 0.0f) {
      const float theta = orbitTheta(std::fabs(dx));
      if (theta >= 1e-7f) {
        const glm::vec3 n = glm::cross(mapToSphere(0.0f, 0.0f), mapToSphere(dx, 0.0f));
        float s = (n.y >= 0.0f) ? 1.0f : -1.0f;
        const glm::vec3 localY = Orientation * glm::vec3(0.0f, 1.0f, 0.0f);
        if (glm::dot(localY, camOY) < 0.0f) s = -s;

        Orientation =
            glm::normalize(Orientation * glm::angleAxis(s * theta, glm::vec3(0.0f, 1.0f, 0.0f)));
        LastTheta = glm::vec4(localY.x, localY.y, localY.z, s * theta);
      }
    }

    // 上下：Orbit 角 + 相机 OX
    if (dy != 0.0f) {
      const float theta = orbitTheta(std::fabs(dy) * VerticalGain);
      if (theta >= 1e-7f) {
        const glm::vec3 n = glm::cross(mapToSphere(0.0f, 0.0f), mapToSphere(0.0f, dy));
        const float s = (n.x >= 0.0f) ? -1.0f : 1.0f;
        const glm::vec3 axis = camOX * s;
        Orientation = glm::normalize(glm::angleAxis(theta, axis) * Orientation);
        LastTheta = glm::vec4(axis.x, axis.y, axis.z, theta);
      }
    }
  }

  /** 抬手占位；mobile 无 OA 缓存，可空实现 */
  void endDrag() {}

  const glm::vec4& lastRotationVector() const { return LastTheta; }
  const glm::quat& orientation() const { return Orientation; }
  glm::mat4 rotationMatrix() const { return glm::mat4_cast(Orientation); }

 private:
  static glm::vec3 safeNorm(const glm::vec3& v, const glm::vec3& fallback) {
    const float len = glm::length(v);
    return (len > 1e-6f) ? (v / len) : fallback;
  }

  /** θ = 2π · px / Height · Sensitivity（OrbitControls） */
  float orbitTheta(float absPixels) const {
    const float h = static_cast<float>(std::max(Height, 1));
    return (absPixels / h) * kTwoPi * Sensitivity;
  }

  /** 仅用于取叉乘符号，不参与幅度 */
  glm::vec3 mapToSphere(float x, float y) const {
    const float r = std::max(Radius, 1.0f);
    float nx = x / r;
    float ny = y / r;
    const float d2 = nx * nx + ny * ny;
    if (d2 <= 1.0f) {
      return glm::normalize(glm::vec3(nx, ny, std::sqrt(1.0f - d2)));
    }
    const float inv = 1.0f / std::sqrt(d2);
    nx *= inv;
    ny *= inv;
    return glm::normalize(glm::vec3(nx, ny, 0.0f));
  }
};

#endif /* CORE_GL_ARCBALL_MOBILE_H_ */
