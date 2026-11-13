//
// arcball_mobile.h — Arcball + mobile axes
//
// 轴向怎么定（begin vs last）：
//   |Δx| 大 → 只用 dx：滚轴 // 物体局部 Y（过中心）
//   |Δy| 大 → 只用 dy：滚轴 // 相机 OX（Right）
//
// 左右不反（主流修法）：物体翻到顶/底后 localY 相对相机 Up 倒置时，
//   若 dot(localY, camUp) < 0，则左右转角取反，保证手指方向与屏幕一致。
//   （更主流的是左右直接绕相机/世界 Up；这里保留物体 Y + 符号修正。）
//
// θ 仍用 Arcball：球极点 OA、本帧 OB → acos(OA·OB)
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

  void setSensitivity(float s) {
    if (s > 1e-3f) sensitivity_ = s;
  }

  float sensitivity() const { return sensitivity_; }

  void reset() {
    orientation_ = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    lastTheta_ = glm::vec4(0.0f);
    endDrag();
  }

  /**
   * @param camRight  相机 OX（Right）— 上下拖滚轴
   * @param camUp     相机 OY（Up）— 用于左右符号修正（防倒置后左右反）
   */
  void drag(float dx, float dy, const glm::vec3& camRight, const glm::vec3& camUp) {
    if (dx == 0.0f && dy == 0.0f) return;

    // begin：抬手后第一帧记下按下点
    if (!gestureActive_) {
      gestureActive_ = true;
      beginX_ = 0.0f;
      beginY_ = 0.0f;
      lastX_ = 0.0f;
      lastY_ = 0.0f;
    }

    // last：累计到当前点
    lastX_ += dx;
    lastY_ += dy;

    // begin vs last → 决定用 dx 还是 dy
    const float absDx = std::fabs(lastX_ - beginX_);
    const float absDy = std::fabs(lastY_ - beginY_);
    const bool useDx = absDx >= absDy;

    float feedX = 0.0f;
    float feedY = 0.0f;
    if (useDx) {
      feedX = dx;
    } else {
      feedY = dy;
    }
    if (feedX == 0.0f && feedY == 0.0f) return;

    const glm::vec3 camOX = safeNorm(camRight, glm::vec3(1.0f, 0.0f, 0.0f));
    const glm::vec3 camOY = safeNorm(camUp, glm::vec3(0.0f, 1.0f, 0.0f));

    const glm::vec3 oa = mapToSphere(0.0f, 0.0f);
    const glm::vec3 ob = useDx ? mapToSphere(feedX, 0.0f) : mapToSphere(0.0f, feedY);

    const float cosTheta = glm::clamp(glm::dot(oa, ob), -1.0f, 1.0f);
    const float theta = std::acos(cosTheta);
    if (theta < 1e-7f) {
      lastTheta_ = glm::vec4(0.0f);
      return;
    }

    const glm::vec3 n = glm::cross(oa, ob);
    if (useDx) {
      // 左右：绕物体局部 +Y；倒置时相对相机 Up 取反，避免顶/底朝向相机时左右反了
      float s = (n.y >= 0.0f) ? 1.0f : -1.0f;
      const glm::vec3 localY = orientation_ * glm::vec3(0.0f, 1.0f, 0.0f);
      if (glm::dot(localY, camOY) < 0.0f) {
        s = -s;
      }
      const glm::quat q = glm::angleAxis(s * theta, glm::vec3(0.0f, 1.0f, 0.0f));
      orientation_ = glm::normalize(orientation_ * q);
      lastTheta_ = glm::vec4(localY.x, localY.y, localY.z, s * theta);
    } else {
      // 上下：滚轴 // 相机 OX（已反向）
      const float s = (n.x >= 0.0f) ? -1.0f : 1.0f;
      const glm::vec3 axis = camOX * s;
      lastTheta_ = glm::vec4(axis.x, axis.y, axis.z, theta);
      orientation_ = glm::normalize(glm::angleAxis(theta, axis) * orientation_);
    }
  }

  /** 抬手：下一笔重新 begin */
  void endDrag() {
    gestureActive_ = false;
    beginX_ = beginY_ = 0.0f;
    lastX_ = lastY_ = 0.0f;
  }

  const glm::vec4& lastRotationVector() const { return lastTheta_; }
  const glm::quat& orientation() const { return orientation_; }
  glm::mat4 rotationMatrix() const { return glm::mat4_cast(orientation_); }

 private:
  static glm::vec3 safeNorm(const glm::vec3& v, const glm::vec3& fallback) {
    const float len = glm::length(v);
    return (len > 1e-6f) ? (v / len) : fallback;
  }

  float effectiveRadius() const {
    return std::max(radius_ / std::max(sensitivity_, 1e-3f), 1.0f);
  }

  glm::vec3 mapToSphere(float x, float y) const {
    const float r = effectiveRadius();
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
  float sensitivity_ = 4.0f;
  bool gestureActive_ = false;
  float beginX_ = 0.0f;
  float beginY_ = 0.0f;
  float lastX_ = 0.0f;
  float lastY_ = 0.0f;
  glm::quat orientation_{1.0f, 0.0f, 0.0f, 0.0f};
  glm::vec4 lastTheta_{0.0f};
};

#endif /* CORE_GL_ARCBALL_MOBILE_H_ */
