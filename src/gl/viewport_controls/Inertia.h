//
// Inertia.h — OrbitControls / Sketchfab 残差阻尼（拖的时候就阻尼，不只是松手）
//
// su7.mov / Sketchfab 真正差在这：
//   输入不立刻整段上模型，而是累进 residual
//   每帧：apply = residual * dampingFactor
//         residual *= (1 - dampingFactor)
//   手指按住不动：模型继续「追上」残差 → 沉
//   松手：同一套残差继续泄 → 自然滑行
//
// 旧方案只在 end 估速再 coast：拖的时候仍是生硬 1:1，对不上 su7。
// 转轴仍由 ArcballMobile 决定；这里只产出平滑后的 (dx,dy)。
//

#ifndef CORE_GL_INERTIA_H_
#define CORE_GL_INERTIA_H_

#include <algorithm>
#include <chrono>
#include <cmath>

class Inertia {
 public:
  using Clock = std::chrono::steady_clock;

  /**
   * @param dampingFactor  每帧吃掉残差的比例（现行默认 0.04；Sketchfab 常 0.05~0.08）
   * @param stopEpsilon    |residual| 低于此且未触摸则停
   * @param unused2/3/4    兼容旧 setParams 形参位置
   */
  void setParams(float dampingFactor, float /*unused*/ = 0.0f,
                 float /*unusedThr*/ = 0.0f, float /*unusedMass*/ = 1.0f,
                 float /*unusedEma*/ = 0.0f, float stopEpsilon = 0.35f) {
    if (dampingFactor > 0.0f && dampingFactor < 1.0f) dampingFactor_ = dampingFactor;
    if (stopEpsilon > 0.0f) stopEpsilon_ = stopEpsilon;
  }

  void setDamping(float dampingFactor) {
    if (dampingFactor > 0.0f && dampingFactor < 1.0f) dampingFactor_ = dampingFactor;
  }

  void reset() {
    touching_ = false;
    active_ = false;
    resX_ = resY_ = 0.0f;
    beginX_ = beginY_ = 0.0f;
    lastX_ = lastY_ = 0.0f;
    lastTickTime_ = Clock::now();
  }

  /** 按下：清残差，打断上一轮滑行 */
  void begin(float x = 0.0f, float y = 0.0f) {
    touching_ = true;
    active_ = true;
    resX_ = resY_ = 0.0f;
    beginX_ = x;
    beginY_ = y;
    lastX_ = x;
    lastY_ = y;
    lastTickTime_ = Clock::now();
  }

  /** 拖动：只累加残差，不直接转模型 */
  void move(float dx, float dy) {
    if (!touching_) begin(0.0f, 0.0f);
    resX_ += dx;
    resY_ += dy;
    lastX_ += dx;
    lastY_ += dy;
    active_ = true;
  }

  /** 抬手：残差继续由 tick 泄完（同一条阻尼曲线） */
  void end() {
    touching_ = false;
    // 残差还在 → active_ 保持，由 tick 收尾
    if (std::fabs(resX_) < stopEpsilon_ && std::fabs(resY_) < stopEpsilon_) {
      resX_ = resY_ = 0.0f;
      active_ = false;
    }
  }

  /**
   * 每帧取出本帧应施加的屏幕位移（已按 dt 折合到 60fps 阻尼强度）。
   * 拖着 / 松手后都走这里 → arcball.drag(outDx, outDy, ...)。
   */
  bool tick(float& outDx, float& outDy) {
    outDx = outDy = 0.0f;
    if (!active_) return false;

    const Clock::time_point now = Clock::now();
    float dt = secondsBetween(lastTickTime_, now);
    if (dt < 1e-4f) dt = 1e-4f;
    if (dt > 0.05f) dt = 0.05f;
    lastTickTime_ = now;

    // 等效 60fps 下每帧吃 dampingFactor_：keep=(1-d)^frames
    const float frames = dt * 60.0f;
    const float applyFrac = 1.0f - std::pow(1.0f - dampingFactor_, frames);
    outDx = resX_ * applyFrac;
    outDy = resY_ * applyFrac;
    resX_ -= outDx;
    resY_ -= outDy;

    if (!touching_ && std::fabs(resX_) < stopEpsilon_ && std::fabs(resY_) < stopEpsilon_) {
      resX_ = resY_ = 0.0f;
      active_ = false;
    }
    return (outDx != 0.0f || outDy != 0.0f) || active_;
  }

  bool coasting() const { return active_ && !touching_; }
  bool touching() const { return touching_; }
  bool active() const { return active_; }

  float beginX() const { return beginX_; }
  float beginY() const { return beginY_; }
  float lastX() const { return lastX_; }
  float lastY() const { return lastY_; }
  float residualX() const { return resX_; }
  float residualY() const { return resY_; }

  // 兼容旧调用名
  float vx() const { return resX_; }
  float vy() const { return resY_; }

 private:
  static float secondsBetween(Clock::time_point a, Clock::time_point b) {
    return std::chrono::duration<float>(b - a).count();
  }

  bool touching_ = false;
  bool active_ = false;
  float resX_ = 0.0f;
  float resY_ = 0.0f;
  float beginX_ = 0.0f;
  float beginY_ = 0.0f;
  float lastX_ = 0.0f;
  float lastY_ = 0.0f;
  Clock::time_point lastTickTime_{};

  // 现行默认（对外 set_orbit_feel 可改）
  float dampingFactor_ = 0.04f;
  float stopEpsilon_ = 0.35f;  // px
};

#endif /* CORE_GL_INERTIA_H_ */
