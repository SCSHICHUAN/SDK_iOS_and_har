//
// explode.h — 爆炸视图
//
// 各零件沿「相对整模中心」方向拉开；进度用 easeOutCubic（与 DebutSequence 同曲线）。
// Draw 时：modelMatrix * translate(offset(i)) * …
//

#ifndef CORE_GL_INTERACTION_EXPLODE_H_
#define CORE_GL_INTERACTION_EXPLODE_H_

#include "ray.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Model;

/**
 * 爆炸 / 复位动画。
 * prepare(Model) 后可 explode() / implode()；每帧 tick(dt)；
 * offset(meshIndex) 给 Render 乘到各 mesh。
 */
class Explode {
 public:
  static constexpr float kDefaultDurationSec = 0.9f;
  static constexpr float kDefaultDistance = 1.0f;  // 相对整模半径的倍数

  bool active() const { return active_; }
  /** 当前爆炸量 0=合拢 … 1=完全拉开（已 ease） */
  float amount() const { return amount_; }
  bool prepared() const { return !parts_.empty(); }
  int partCount() const { return static_cast<int>(parts_.size()); }

  /**
   * 功能：根据 Model 各 mesh 顶点算零件中心与爆炸方向。
   * 输入：model 已 loaded；顶点应为绘制空间（静态 bake / 或蒙皮局部）
   */
  void prepare(const Model& model);

  /** 拉开到 factor（相对半径倍数，默认 1） */
  void explode(float factor = kDefaultDistance, float durationSec = kDefaultDurationSec) {
    if (parts_.empty()) return;
    active_ = true;
    t_ = 0.0f;
    duration_ = std::max(durationSec, 0.05f);
    amount0_ = amount_;
    amount1_ = std::max(factor, 0.0f);
  }

  /** 收回到合拢 */
  void implode(float durationSec = kDefaultDurationSec) {
    explode(0.0f, durationSec);
  }

  void tick(float dt) {
    if (!active_) return;
    t_ += std::max(dt, 0.0f);
    float x = t_ / duration_;
    if (x >= 1.0f) {
      x = 1.0f;
      active_ = false;
    }
    const float y = easeOutCubic(x);
    amount_ = amount0_ + (amount1_ - amount0_) * y;
  }

  /** 第 meshIndex 零件的平移（模型空间，已含 amount） */
  glm::vec3 offset(int meshIndex) const {
    if (meshIndex < 0 || meshIndex >= static_cast<int>(parts_.size())) {
      return glm::vec3(0.0f);
    }
    return parts_[meshIndex].dir * (parts_[meshIndex].dist * amount_);
  }

  /** 便于 Draw：I + translate(offset) */
  glm::mat4 offsetMatrix(int meshIndex) const {
    return glm::translate(glm::mat4(1.0f), offset(meshIndex));
  }

  void reset() {
    active_ = false;
    amount_ = 0.0f;
    amount0_ = amount1_ = 0.0f;
    t_ = 0.0f;
  }

  void clear() {
    reset();
    parts_.clear();
    modelRadius_ = 1.0f;
  }

 private:
  struct Part {
    glm::vec3 dir{0.0f, 1.0f, 0.0f};  // 单位方向（相对整模中心）
    float dist = 0.0f;                 // 完全爆炸时位移长度
  };

  static float easeOutCubic(float x) {
    x = std::clamp(x, 0.0f, 1.0f);
    const float u = 1.0f - x;
    return 1.0f - u * u * u;
  }

  bool active_ = false;
  float t_ = 0.0f;
  float duration_ = kDefaultDurationSec;
  float amount0_ = 0.0f;
  float amount1_ = 0.0f;
  float amount_ = 0.0f;
  float modelRadius_ = 1.0f;
  std::vector<Part> parts_;
};

#endif /* CORE_GL_INTERACTION_EXPLODE_H_ */
