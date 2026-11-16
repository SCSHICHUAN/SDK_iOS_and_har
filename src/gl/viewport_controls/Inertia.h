//
// inertia.h — 拖动 100% 跟手；松手刹车与 Sensitivity 解耦
//
// 与旧「残差阻尼」的区别：
//   旧：拖着 / 松手共用一个 damping → 改刹车就拖不动
//   现：
//     ③ 拖动：move 把 dx 全量进 Pending，tick 全量吐出 → 跟手由 Arcball Sensitivity 决定
//     ④ 松手：end 近窗估速启动 Coast；tick 里 v*=(1-BrakeFactor) → 只管滑行长短
//
// 转轴仍由 ArcballMobile 决定；这里只产出平滑后的 (dx,dy)。
//

#ifndef CORE_GL_INERTIA_H_
#define CORE_GL_INERTIA_H_

#include <algorithm>
#include <chrono>
#include <cmath>

/** 每「帧」(按 60fps 折合) 速度保留比例的补：v *= (1 - brake)^frames；越大刹越快 */
const float kInertiaBrakeFactor = 0.10f;
/** 松手瞬时速度低于此则不启动滑行（像素/秒量级） */
const float kInertiaSpeedThreshold = 60.0f;
/** 滑行中速度低于此则停 */
const float kInertiaStopSpeed = 22.0f;

class Inertia {
 public:
  using Clock = std::chrono::steady_clock;

  static constexpr int kMaxSamples = 24;

  /** 近窗估速用的瞬时速度样本 */
  struct Sample {
    float vx = 0.0f;
    float vy = 0.0f;
    Clock::time_point t{};
  };

  // —— members ——
  bool Touching;   // 手指是否按下
  bool Coasting;   // 是否在松手滑行
  bool HasSample;  // 是否采过速度（用于 end 判 fling）

  float PendingDx;  // 本帧尚未吐给 arcball 的拖动残差 X（拖动路径 100% 入队）
  float PendingDy;
  float BeginX;     // begin 时屏幕坐标（调试/对外查询）
  float BeginY;
  float LastX;      // 累计光标（相对 begin 的位移轨迹）
  float LastY;

  float Vx;  // EMA 拖动速度（像素/秒）
  float Vy;
  Clock::time_point LastSampleTime;
  Sample Samples[kMaxSamples];
  int SampleCount;
  int SampleWrite;

  float CoastVx;  // 松手后滑行速度
  float CoastVy;
  Clock::time_point LastTickTime;

  float BrakeFactor;     // 仅松手刹车；与 Sensitivity 无关
  float SpeedThreshold;  // 低于此不启动 coast
  float StopSpeed;       // coast 停速

  // —— methods ——
  Inertia()
      : Touching(false),
        Coasting(false),
        HasSample(false),
        PendingDx(0.0f),
        PendingDy(0.0f),
        BeginX(0.0f),
        BeginY(0.0f),
        LastX(0.0f),
        LastY(0.0f),
        Vx(0.0f),
        Vy(0.0f),
        SampleCount(0),
        SampleWrite(0),
        CoastVx(0.0f),
        CoastVy(0.0f),
        BrakeFactor(kInertiaBrakeFactor),
        SpeedThreshold(kInertiaSpeedThreshold),
        StopSpeed(kInertiaStopSpeed) {
    LastTickTime = Clock::now();
  }

  /**
   * 兼容旧 setParams 形参位置；现行只用 brake / speedThreshold / stopSpeed。
   * @param brakeFactor  松手每帧刹多少（0~1，默认 0.10）
   */
  void setParams(float brakeFactor, float /*unused*/ = 0.0f, float speedThreshold = 80.0f,
                 float /*unusedMass*/ = 1.0f, float /*unusedEma*/ = 0.0f,
                 float stopSpeed = 25.0f) {
    setBrake(brakeFactor);
    if (speedThreshold >= 0.0f) SpeedThreshold = speedThreshold;
    if (stopSpeed > 0.0f) StopSpeed = stopSpeed;
  }

  /** 旧名兼容 → setBrake */
  void setDamping(float brakeFactor) { setBrake(brakeFactor); }

  /** 只改松手刹车，不影响拖动跟手 */
  void setBrake(float brakeFactor) {
    if (brakeFactor > 0.0f && brakeFactor < 1.0f) BrakeFactor = brakeFactor;
  }

  float brakeFactor() const { return BrakeFactor; }

  /** 清空拖动 / 滑行状态 */
  void reset() {
    Touching = false;
    Coasting = false;
    PendingDx = PendingDy = 0.0f;
    CoastVx = CoastVy = 0.0f;
    Vx = Vy = 0.0f;
    BeginX = BeginY = 0.0f;
    LastX = LastY = 0.0f;
    HasSample = false;
    SampleCount = 0;
    SampleWrite = 0;
    LastTickTime = Clock::now();
  }

  /** 按下：打断 coast，清 pending / 速度样本 */
  void begin(float x = 0.0f, float y = 0.0f) {
    Coasting = false;
    CoastVx = CoastVy = 0.0f;
    Touching = true;
    PendingDx = PendingDy = 0.0f;
    BeginX = x;
    BeginY = y;
    LastX = x;
    LastY = y;
    Vx = Vy = 0.0f;
    HasSample = false;
    SampleCount = 0;
    SampleWrite = 0;
    LastSampleTime = Clock::now();
    LastTickTime = LastSampleTime;
  }

  /**
   * 拖动：dx/dy 100% 入 Pending（跟手）；同时采样速度供松手 fling。
   * 不乘 Sensitivity——比例只在 ArcballMobile::drag 里算。
   */
  void move(float dx, float dy) {
    if (!Touching) begin(0.0f, 0.0f);
    Coasting = false;
    CoastVx = CoastVy = 0.0f;

    PendingDx += dx;
    PendingDy += dy;
    LastX += dx;
    LastY += dy;

    const Clock::time_point now = Clock::now();
    float dt = HasSample ? secondsBetween(LastSampleTime, now) : (1.0f / 60.0f);
    if (dt < 1e-4f) dt = 1e-4f;
    if (dt > 0.1f) dt = 0.1f;
    const float instVx = dx / dt;
    const float instVy = dy / dt;
    const float alpha = 0.45f;
    if (HasSample) {
      Vx = alpha * instVx + (1.0f - alpha) * Vx;
      Vy = alpha * instVy + (1.0f - alpha) * Vy;
    } else {
      Vx = instVx;
      Vy = instVy;
    }
    pushSample(instVx, instVy, now);
    LastSampleTime = now;
    HasSample = true;
  }

  /**
   * 抬手：近窗加权估速；高于 SpeedThreshold 则启动 Coast。
   * 低速直接停，避免轻点也滑。
   */
  void end() {
    Touching = false;

    float flingVx = Vx;
    float flingVy = Vy;
    blendVelocityInWindow(0.08f, flingVx, flingVy);
    const float speed = std::sqrt(flingVx * flingVx + flingVy * flingVy);
    if (!HasSample || speed < SpeedThreshold) {
      Coasting = false;
      CoastVx = CoastVy = 0.0f;
      return;
    }

    CoastVx = flingVx;
    CoastVy = flingVy;
    Coasting = true;
    LastTickTime = Clock::now();
  }

  /**
   * 每帧：吐出 Pending（拖动）+ 若 Coasting 则叠加 v·dt 并刹车。
   * 结果交给 arcball.drag(outDx, outDy, ...)。
   * @return 本帧有位移或仍在滑行
   */
  bool tick(float& outDx, float& outDy) {
    outDx = outDy = 0.0f;

    const Clock::time_point now = Clock::now();
    float dt = secondsBetween(LastTickTime, now);
    if (dt < 1e-4f) dt = 1e-4f;
    if (dt > 0.05f) dt = 0.05f;
    LastTickTime = now;

    // 拖动路径：全量吐出 pending（100% 跟手）
    outDx += PendingDx;
    outDy += PendingDy;
    PendingDx = PendingDy = 0.0f;

    // 松手滑行：位移 += v·dt，再按 BrakeFactor 衰减
    if (Coasting && !Touching) {
      outDx += CoastVx * dt;
      outDy += CoastVy * dt;

      const float frames = dt * 60.0f;
      const float keep = std::pow(1.0f - BrakeFactor, frames);
      CoastVx *= keep;
      CoastVy *= keep;

      const float speed = std::sqrt(CoastVx * CoastVx + CoastVy * CoastVy);
      if (speed < StopSpeed) {
        CoastVx = CoastVy = 0.0f;
        Coasting = false;
      }
    }

    return (outDx != 0.0f || outDy != 0.0f) || Coasting;
  }

  bool coasting() const { return Coasting; }
  bool touching() const { return Touching; }
  bool active() const {
    return Coasting || PendingDx != 0.0f || PendingDy != 0.0f || Touching;
  }

  float beginX() const { return BeginX; }
  float beginY() const { return BeginY; }
  float lastX() const { return LastX; }
  float lastY() const { return LastY; }
  float vx() const { return Vx; }
  float vy() const { return Vy; }

 private:
  void pushSample(float vx, float vy, Clock::time_point t) {
    Samples[SampleWrite] = Sample{vx, vy, t};
    SampleWrite = (SampleWrite + 1) % kMaxSamples;
    if (SampleCount < kMaxSamples) ++SampleCount;
  }

  /** 近 windowSec 内样本按「越近权重越大」混合，得到松手瞬时速度 */
  void blendVelocityInWindow(float windowSec, float& outVx, float& outVy) const {
    if (SampleCount <= 0) return;
    const Clock::time_point now = LastSampleTime;
    float wSum = 0.0f;
    float sx = 0.0f;
    float sy = 0.0f;
    for (int i = 0; i < SampleCount; ++i) {
      const int idx = (SampleWrite - 1 - i + kMaxSamples * 4) % kMaxSamples;
      const Sample& s = Samples[idx];
      const float age = secondsBetween(s.t, now);
      if (age > windowSec) break;
      const float w = 1.0f - (age / windowSec);
      sx += s.vx * w;
      sy += s.vy * w;
      wSum += w;
    }
    if (wSum > 1e-6f) {
      outVx = sx / wSum;
      outVy = sy / wSum;
    }
  }

  static float secondsBetween(Clock::time_point a, Clock::time_point b) {
    return std::chrono::duration<float>(b - a).count();
  }
};

#endif /* CORE_GL_INERTIA_H_ */
