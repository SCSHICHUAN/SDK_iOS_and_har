//
// arcball.h — from sdk create by stan 2026/11/12
// Arcball: screen drag → virtual sphere OA/OB → rotation vector θ → model pose
// 轨迹球：屏幕拖拽 → 虚拟球面 OA/OB → 旋转矢量 θ → 模型姿态
//
// From two unit vectors OA, OB on the sphere, derive θ:
// 由球面两个单位向量 OA、OB，推导矢量 θ：
// 1. Cross: n = OA × OB → unit axis
//    叉乘：n = OA × OB → 单位转轴
// 2. Dot: OA · OB = cosθ → angle (radians)
//    点乘：OA · OB = cosθ → 转角（弧度）
// 3. Pack (x,y,z,θ); accumulate pose with quaternion (no Euler)
//    组合：(x,y,z,θ)；姿态用四元数累加（无欧拉）
//
// Feel (mainstream):
// 手感（主流）：
//   - Ball radius ≈ half of viewport short side (setViewport)
//     虚拟球半径 ≈ 视口短边一半（setViewport）
//   - Optional sensitivity: r_eff = r / sensitivity
//     可选 sensitivity：等效缩小球半径 r_eff = r / sensitivity
//   - Same θ written to lastTheta and angleAxis (no extra gain)
//     θ 算出后原样写入 lastTheta 与 angleAxis，不再另乘裸系数
//
// Ball center sits on a plane parallel to the viewport / camera XY;
// hemisphere bulges toward the viewer (+Z). Rotation is later applied
// about model AABB center in RendererData — not about this ball center.
// 球心落在与视口 / 相机 XY 平行的平面上；半球朝向观察者（+Z）鼓起。
// 旋转稍后在 RendererData 里绕模型 AABB 中心施加，不是绕本球心。
//

#ifndef CORE_GL_ARCBALL_H_
#define CORE_GL_ARCBALL_H_

#include <algorithm>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

class Arcball {
 public:
  /**
   * Viewport size in pixels; ball radius = half of the short side.
   * 视口像素尺寸；球半径 = 短边的一半。
   */
  void setViewport(int width, int height) {
    width_ = std::max(width, 1);
    height_ = std::max(height, 1);
    radius_ = 0.5f * static_cast<float>(std::min(width_, height_));
  }

  /**
   * Ball radius in pixels — primary feel knob (larger = slower).
   * 球半径（像素）— 主要手感旋钮（越大转得越慢）。
   */
  void setRadius(float pixels) {
    if (pixels > 1.0f) radius_ = pixels;
  }

  float radius() const { return radius_; }

  /**
   * Drag sensitivity (mainstream secondary knob). Default 1.
   * >1 faster: effective radius = radius / sensitivity.
   * Do not also multiply θ by a separate gain.
   * 拖拽灵敏度（主流次要旋钮）。默认 1。
   * >1 更快：有效半径 = radius / sensitivity。
   * 不要再对 θ 另乘一层增益。
   */
  void setSensitivity(float s) {
    if (s > 1e-3f) sensitivity_ = s;
  }

  float sensitivity() const { return sensitivity_; }

  /**
   * Reset accumulated model orientation.
   * 重置累计的模型朝向。
   */
  void reset() {
    orientation_ = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    lastTheta_ = glm::vec4(0.0f);
    haveLast_ = false;
  }

  /**
   * One drag frame from Pro (pixel deltas). Builds OA/OB on the virtual
   * hemisphere, derives rotation vector (axis, θ), composites into pose.
   * Screen Y is flipped so finger-up matches typical GL feel.
   * Pro 传来的一帧拖拽（像素增量）。在虚拟半球上构造 OA/OB，
   * 求出旋转矢量（轴, θ），复合进总姿态。
   * 翻转屏幕 Y，使手指上滑符合常见 GL 手感。
   */
  void drag(float dx, float dy) {
    if (dx == 0.0f && dy == 0.0f) return;

    if (!haveLast_) {
      cursorX_ = 0.0f;
      cursorY_ = 0.0f;
      lastSphere_ = mapToSphere(cursorX_, cursorY_);
      haveLast_ = true;
    }

    const float nextX = cursorX_ + dx;
    const float nextY = cursorY_ - dy;  // screen Y down → ball Y up / 屏Y向下 → 球Y向上
    const glm::vec3 oa = lastSphere_;
    const glm::vec3 ob = mapToSphere(nextX, nextY);

    cursorX_ = nextX;
    cursorY_ = nextY;
    lastSphere_ = ob;

    applySphereRotation(oa, ob);
  }

  /**
   * Call when the finger lifts so the next gesture re-seeds OA.
   * 手指抬起时调用，下一手势重新 seed OA。
   */
  void endDrag() { haveLast_ = false; }

  /**
   * Last incremental rotation vector: (axis.x, axis.y, axis.z, θ radians).
   * 上一帧增量旋转矢量：(轴x, 轴y, 轴z, θ 弧度)。
   */
  const glm::vec4& lastRotationVector() const { return lastTheta_; }

  /**
   * Total model orientation as quaternion.
   * 模型总朝向（四元数）。
   */
  const glm::quat& orientation() const { return orientation_; }

  /**
   * Rotation for the model matrix (about model center).
   * 供模型矩阵使用的旋转（绕模型中心施加）。
   */
  glm::mat4 rotationMatrix() const { return glm::mat4_cast(orientation_); }

 private:
  float effectiveRadius() const {
    return std::max(radius_ / std::max(sensitivity_, 1e-3f), 1.0f);
  }

  /**
   * Project ball-local pixel (x,y) onto the unit hemisphere.
   * Outside the disk → unit vector on the equator plane.
   * 将球局部像素 (x,y) 投射到单位半球上。
   * 落在圆盘外 → 赤道平面上的单位向量。
   */
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

  void applySphereRotation(const glm::vec3& oa, const glm::vec3& ob) {
    glm::vec3 axis = glm::cross(oa, ob); // 叉得到旋转轴
    const float axisLen = glm::length(axis);
    if (axisLen < 1e-6f) {
      lastTheta_ = glm::vec4(0.0f);
      return;
    }
    axis /= axisLen;

    // 对点乘结果限制在 [-1,1] 区间，得到夹角的余弦值
   const float cosTheta = glm::clamp(glm::dot(oa, ob), -1.0f, 1.0f);
    // 反余弦，求出两个向量之间的夹角（弧度制）
   const float theta = std::acos(cosTheta);

    if (theta < 1e-7f) {
      lastTheta_ = glm::vec4(0.0f);
      return;
    }

    // Same θ for record and pose — no separate output gain.
    // 记录与施加使用同一 θ — 不再另加输出增益。
    lastTheta_ = glm::vec4(axis.x, axis.y, axis.z, theta);
    const glm::quat delta = glm::angleAxis(theta, axis);
    orientation_ = glm::normalize(delta * orientation_);
  }

  int width_ = 1;
  int height_ = 1;
  float radius_ = 200.0f;
  float sensitivity_ = 2.5f;
  float cursorX_ = 0.0f;
  float cursorY_ = 0.0f;
  glm::vec3 lastSphere_{0.0f, 0.0f, 1.0f};
  bool haveLast_ = false;
  glm::quat orientation_{1.0f, 0.0f, 0.0f, 0.0f};
  glm::vec4 lastTheta_{0.0f};
};

#endif /* CORE_GL_ARCBALL_H_ */
