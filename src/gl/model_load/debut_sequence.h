//
// debut_sequence.h — 加载后入场
//
// 单曲线 easeOutCubic（管收尾）：y = 1-(1-x)³
//   前半出手快，后半减速贴停，收尾不「卡住」。
// 绕 Y：由 Model::preferredFrontYawOffset() + 展示角决定（见 ViewportInput）。
// 默认展示：逆时针约 135°，结束正面偏 45° 三分斜视；缩放同进度；不可打断。
//

#ifndef CORE_GL_DEBUT_SEQUENCE_H_
#define CORE_GL_DEBUT_SEQUENCE_H_

#include <algorithm>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "arcball_mobile.h"

class DebutSequence {
 public:
  static constexpr float kDurationSec = 1.4f;
  static constexpr float kStartScaleRatio = 0.20f;
  /** 结束时相对「正面」的偏航（三分斜视） */
  static constexpr float kPresentYawRad = 0.78539816339f;  // +45°
  /** 进场转角幅度（逆时针，从侧视转到 present） */
  static constexpr float kSpinRad = 2.35619449019f;        // 135°
  // 兼容旧常量名（无 front 校正时的绝对角）
  static constexpr float kYawEndRad = kPresentYawRad;
  static constexpr float kYawStartRad = kPresentYawRad - kSpinRad;  // -90°

  bool active() const { return active_; }
  bool blocksInput() const { return active_; }
  bool interruptible() const { return false; }

  float distance() const { return distance_; }
  float scale() const { return scale_; }
  float yawRadians() const { return yaw_; }

  void setDistance(float distance) {
    dist_ = std::max(distance, 0.1f);
    distance_ = dist_;
  }

  void start(float scaleStart, float scaleEnd, float distance, float yawStartRad,
             float yawEndRad, float durationSec = kDurationSec) {
    active_ = true;
    t_ = 0.0f;
    duration_ = std::max(durationSec, 0.05f);
    scale0_ = std::max(scaleStart, 1e-4f);
    scale1_ = std::max(scaleEnd, 1e-4f);
    dist_ = std::max(distance, 0.1f);
    yaw0_ = yawStartRad;
    yaw1_ = yawEndRad;
    scale_ = scale0_;
    distance_ = dist_;
    yaw_ = yaw0_;
  }

  void tick(float dt) {
    if (!active_) return;
    t_ += std::max(dt, 0.0f);
    float x = t_ / duration_;
    if (x >= 1.0f) {
      x = 1.0f;
      active_ = false;
    }
    const float y = easeOutCubic(x);  // 收尾减速
    scale_ = scale0_ + (scale1_ - scale0_) * y;
    yaw_ = yaw0_ + (yaw1_ - yaw0_) * y;
    distance_ = dist_;
  }

  void settle(ArcballMobile& arcball, float& modelScale) {
    if (std::fabs(yaw_) > 1e-6f) {
      const glm::quat spinQ = glm::angleAxis(yaw_, glm::vec3(0.0f, 1.0f, 0.0f));
      arcball.Orientation = glm::normalize(spinQ * arcball.Orientation);
      yaw_ = 0.0f;
    }
    modelScale = scale_;
    active_ = false;
  }

  void cancel(ArcballMobile& arcball, float& modelScale) { settle(arcball, modelScale); }

 private:
  /** 管收尾：1-(1-x)³，末端速度归零、不硬刹 */
  static float easeOutCubic(float x) {
    x = std::clamp(x, 0.0f, 1.0f);
    const float u = 1.0f - x;
    return 1.0f - u * u * u;
  }

  bool active_ = false;
  float t_ = 0.0f;
  float duration_ = kDurationSec;
  float scale0_ = 0.20f;
  float scale1_ = 1.0f;
  float dist_ = 3.0f;
  float yaw0_ = kYawStartRad;
  float yaw1_ = kYawEndRad;
  float scale_ = 0.20f;
  float distance_ = 3.0f;
  float yaw_ = kYawStartRad;
};

#endif /* CORE_GL_DEBUT_SEQUENCE_H_ */
