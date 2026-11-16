//
// arcball.h — full arcball: OA/OB → θ → quaternion pose
// 完整轨迹球：屏幕拖拽 → 虚拟球面 OA/OB → 旋转矢量 θ → 模型姿态
//
// 由球面两个单位向量 OA、OB 推导 θ：
//   1. 叉乘 n = OA × OB → 单位转轴
//   2. 点乘 OA · OB = cosθ → 转角（弧度）
//   3. 打包 (x,y,z,θ)；姿态用四元数累加（不用欧拉）
//
// 手感：
//   - 球半径 ≈ 视口短边一半（setViewport）
//   - Sensitivity：r_eff = Radius / Sensitivity（越大转越快）
//   - θ 算出后原样写入 LastTheta 与 angleAxis，不再另乘增益
//
// 球心落在与视口 / 相机 XY 平行的平面上；半球朝观察者（+Z）。
// 旋转在 RendererData 里绕模型 AABB 中心施加，不是绕本球心。
//

#ifndef CORE_GL_ARCBALL_H_
#define CORE_GL_ARCBALL_H_

#include <algorithm>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

/** 默认拖拽灵敏度；>1 等效缩小球半径，同位移转角更大 */
const float kArcballSensitivity = 2.5f;

class Arcball {
 public:
  // —— members ——
  int Width;              // 视口宽（像素）
  int Height;             // 视口高（像素）
  float Radius;           // 虚拟球半径（像素）；越大转得越慢
  float Sensitivity;      // 次要手感：r_eff = Radius / Sensitivity
  float CursorX;          // 相对拖拽起点的球平面 X
  float CursorY;          // 相对拖拽起点的球平面 Y（屏幕 Y 下 → 球 Y 上已在 drag 里翻转）
  glm::vec3 LastSphere;   // 上一帧球面点 OB（下一帧的 OA）
  bool HaveLast;          // 是否已有上一球面点（抬手后清掉）
  glm::quat Orientation;  // 累计模型朝向
  glm::vec4 LastTheta;    // 最近一次 (axis.xyz, θ)

  // —— methods ——
  Arcball()
      : Width(1),
        Height(1),
        Radius(200.0f),
        Sensitivity(kArcballSensitivity),
        CursorX(0.0f),
        CursorY(0.0f),
        LastSphere(0.0f, 0.0f, 1.0f),
        HaveLast(false),
        Orientation(1.0f, 0.0f, 0.0f, 0.0f),
        LastTheta(0.0f) {}

  /** 视口像素尺寸；球半径 = 短边的一半 */
  void setViewport(int width, int height) {
    Width = std::max(width, 1);
    Height = std::max(height, 1);
    Radius = 0.5f * static_cast<float>(std::min(Width, Height));
  }

  /** 球半径（像素）— 主要手感旋钮（越大转得越慢） */
  void setRadius(float pixels) {
    if (pixels > 1.0f) Radius = pixels;
  }

  float radius() const { return Radius; }

  /**
   * 拖拽灵敏度（次要旋钮）。默认见 kArcballSensitivity。
   * >1 更快：有效半径 = Radius / Sensitivity。不要再对 θ 另乘增益。
   */
  void setSensitivity(float s) {
    if (s > 1e-3f) Sensitivity = s;
  }

  float sensitivity() const { return Sensitivity; }

  /** 重置累计朝向与拖拽状态 */
  void reset() {
    Orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    LastTheta = glm::vec4(0.0f);
    HaveLast = false;
  }

  /**
   * 屏幕位移 (dx,dy) → 球面 OA/OB → 四元数累加。
   * 屏幕 Y 向下，映射球时取反。
   */
  void drag(float dx, float dy) {
    if (dx == 0.0f && dy == 0.0f) return;

    if (!HaveLast) {
      CursorX = 0.0f;
      CursorY = 0.0f;
      LastSphere = mapToSphere(CursorX, CursorY);
      HaveLast = true;
    }

    const float nextX = CursorX + dx;
    const float nextY = CursorY - dy;  // screen Y down → ball Y up
    const glm::vec3 oa = LastSphere;
    const glm::vec3 ob = mapToSphere(nextX, nextY);

    CursorX = nextX;
    CursorY = nextY;
    LastSphere = ob;

    applySphereRotation(oa, ob);
  }

  /** 抬手：下次 drag 重新从球心附近起算 OA */
  void endDrag() { HaveLast = false; }

  const glm::vec4& lastRotationVector() const { return LastTheta; }
  const glm::quat& orientation() const { return Orientation; }
  glm::mat4 rotationMatrix() const { return glm::mat4_cast(Orientation); }

 private:
  /** 有效球半径：Sensitivity 越大，球越小，同位移 θ 越大 */
  float effectiveRadius() const {
    return std::max(Radius / std::max(Sensitivity, 1e-3f), 1.0f);
  }

  /** 平面点映射到单位半球；球外投影到赤道圆上 */
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

  /** OA×OB → 轴；acos(OA·OB) → θ；左乘四元数更新 Orientation */
  void applySphereRotation(const glm::vec3& oa, const glm::vec3& ob) {
    glm::vec3 axis = glm::cross(oa, ob);
    const float axisLen = glm::length(axis);
    if (axisLen < 1e-6f) {
      LastTheta = glm::vec4(0.0f);
      return;
    }
    axis /= axisLen;

    const float cosTheta = glm::clamp(glm::dot(oa, ob), -1.0f, 1.0f);
    const float theta = std::acos(cosTheta);
    if (theta < 1e-7f) {
      LastTheta = glm::vec4(0.0f);
      return;
    }

    LastTheta = glm::vec4(axis.x, axis.y, axis.z, theta);
    const glm::quat delta = glm::angleAxis(theta, axis);
    Orientation = glm::normalize(delta * Orientation);
  }
};

#endif /* CORE_GL_ARCBALL_H_ */
