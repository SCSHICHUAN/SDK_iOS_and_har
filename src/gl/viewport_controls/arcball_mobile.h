//
// arcball_mobile.h — 移动端物体旋转（齿轮放大，不是跟手封顶）
//
//   dx → 物体局部 Y；dy → 相机 OX；可同时
//
// 转角：
//   θ = (|像素增量| / radius) * sensitivity_
//   sensitivity_ = 齿轮比：1≈接近跟手，8~20 = 动一点转很多
//   转向用 OA×OB 的符号；局部 Y 倒置时左右取反
//

#ifndef CORE_GL_ARCBALL_MOBILE_H_
#define CORE_GL_ARCBALL_MOBILE_H_

#include <algorithm>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

class ArcballMobile {
 public:
  void setViewport(int width, int height) {
    width_ = std::max(width, 1);
    height_ = std::max(height, 1);
    radius_ = 0.5f * static_cast<float>(std::min(width_, height_));
  }

  void setRadius(float pixels) {
    if (pixels > 1.0f) radius_ = pixels;
  }

  float radius() const { return radius_; }

  /**
   * 齿轮比（不是挤球半径）。
   * 1 ≈ 跟手；越大 = 同样拖动模型转得越多（可明显超过跟手）。
   */
  void setSensitivity(float s) {
    if (s > 1e-3f) sensitivity_ = s;
  }

  float sensitivity() const { return sensitivity_; }

  /** 上下相对左右的增益 */
  void setVerticalGain(float g) {
    if (g > 1e-3f) verticalGain_ = g;
  }

  float verticalGain() const { return verticalGain_; }

  void reset() {
    orientation_ = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    lastTheta_ = glm::vec4(0.0f);
  }

  void drag(float dx, float dy, const glm::vec3& camRight, const glm::vec3& camUp) {
    if (dx == 0.0f && dy == 0.0f) return;

    const glm::vec3 camOX = safeNorm(camRight, glm::vec3(1.0f, 0.0f, 0.0f));
    const glm::vec3 camOY = safeNorm(camUp, glm::vec3(0.0f, 1.0f, 0.0f));
    const float r = std::max(radius_, 1.0f);

    // 左右：齿轮角 + 符号（局部 Y / 倒置修正）
    if (dx != 0.0f) {
      const float theta = gearedTheta(std::fabs(dx), r);
      if (theta >= 1e-7f) {
        const glm::vec3 n = glm::cross(mapToSphere(0.0f, 0.0f), mapToSphere(dx, 0.0f));
        float s = (n.y >= 0.0f) ? 1.0f : -1.0f;
        const glm::vec3 localY = orientation_ * glm::vec3(0.0f, 1.0f, 0.0f);
        if (glm::dot(localY, camOY) < 0.0f) s = -s;

        orientation_ =
            glm::normalize(orientation_ * glm::angleAxis(s * theta, glm::vec3(0.0f, 1.0f, 0.0f)));
        lastTheta_ = glm::vec4(localY.x, localY.y, localY.z, s * theta);
      }
    }

    // 上下：齿轮角 + 相机 OX
    if (dy != 0.0f) {
      const float theta = gearedTheta(std::fabs(dy) * verticalGain_, r);
      if (theta >= 1e-7f) {
        const glm::vec3 n = glm::cross(mapToSphere(0.0f, 0.0f), mapToSphere(0.0f, dy));
        const float s = (n.x >= 0.0f) ? -1.0f : 1.0f;
        const glm::vec3 axis = camOX * s;
        orientation_ = glm::normalize(glm::angleAxis(theta, axis) * orientation_);
        lastTheta_ = glm::vec4(axis.x, axis.y, axis.z, theta);
      }
    }
  }

  void endDrag() {}

  const glm::vec4& lastRotationVector() const { return lastTheta_; }
  const glm::quat& orientation() const { return orientation_; }
  glm::mat4 rotationMatrix() const { return glm::mat4_cast(orientation_); }

 private:
  static glm::vec3 safeNorm(const glm::vec3& v, const glm::vec3& fallback) {
    const float len = glm::length(v);
    return (len > 1e-6f) ? (v / len) : fallback;
  }

  float gearedTheta(float absPixels, float r) const {
    return (absPixels / r) * sensitivity_;
  }

  /** 仅用于取叉乘符号，不参与幅度 */
  glm::vec3 mapToSphere(float x, float y) const {
    const float r = std::max(radius_, 1.0f);
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

  int width_ = 1;
  int height_ = 1;
  float radius_ = 200.0f;
  float sensitivity_ = 15.0f;  // 现行默认；对外 set_orbit_feel 可改
  float verticalGain_ = 0.85f;
  glm::quat orientation_{1.0f, 0.0f, 0.0f, 0.0f};
  glm::vec4 lastTheta_{0.0f};
};

#endif /* CORE_GL_ARCBALL_MOBILE_H_ */
