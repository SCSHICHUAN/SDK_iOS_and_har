//
// ray.h — 射线（点选 Pick 用）
//
// 屏幕点 → 世界射线；与 AABB 求交。点击选取至少需要这一层。
//

#ifndef CORE_GL_INTERACTION_RAY_H_
#define CORE_GL_INTERACTION_RAY_H_

#include <algorithm>
#include <cmath>
#include <limits>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Ray {
  glm::vec3 origin{0.0f};
  glm::vec3 direction{0.0f, 0.0f, -1.0f};  // 单位向量

  /**
   * 功能：从屏幕像素生成世界空间射线。
   * 参数：sx/sy — 像素（左上为 0,0，与 UIKit 一致）；w/h — 视口；
   *       invViewProj — inverse(projection * view)
   */
  static Ray fromScreen(float sx, float sy, int w, int h, const glm::mat4& invViewProj) {
    Ray r;
    if (w <= 0 || h <= 0) return r;
    // NDC：x[-1,1] y[-1,1]（OpenGL 底为 -1；sy 自上而下 → 翻转）
    const float ndcX = (2.0f * sx) / static_cast<float>(w) - 1.0f;
    const float ndcY = 1.0f - (2.0f * sy) / static_cast<float>(h);
    const glm::vec4 nearH = invViewProj * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
    const glm::vec4 farH = invViewProj * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
    const glm::vec3 nearP = glm::vec3(nearH) / std::max(nearH.w, 1e-8f);
    const glm::vec3 farP = glm::vec3(farH) / std::max(farH.w, 1e-8f);
    r.origin = nearP;
    const glm::vec3 dir = farP - nearP;
    const float len = glm::length(dir);
    r.direction = (len > 1e-8f) ? (dir / len) : glm::vec3(0.0f, 0.0f, -1.0f);
    return r;
  }

  /**
   * 功能：射线与轴对齐盒求交。
   * 返回：true 命中；tHit 为沿射线最近点距离（≥0）
   */
  bool intersectAABB(const glm::vec3& bmin, const glm::vec3& bmax, float& tHit) const {
    float tmin = 0.0f;
    float tmax = std::numeric_limits<float>::max();
    for (int i = 0; i < 3; ++i) {
      const float o = origin[i];
      const float d = direction[i];
      if (std::fabs(d) < 1e-8f) {
        if (o < bmin[i] || o > bmax[i]) return false;
        continue;
      }
      float t0 = (bmin[i] - o) / d;
      float t1 = (bmax[i] - o) / d;
      if (t0 > t1) std::swap(t0, t1);
      tmin = std::max(tmin, t0);
      tmax = std::min(tmax, t1);
      if (tmax < tmin) return false;
    }
    if (tmax < 0.0f) return false;
    tHit = (tmin >= 0.0f) ? tmin : tmax;
    return true;
  }
};

/** 零件 AABB（模型空间，供爆炸方向 / 点选） */
struct PartBounds {
  glm::vec3 bmin{0.0f};
  glm::vec3 bmax{0.0f};
  glm::vec3 center{0.0f};
  bool valid = false;

  void expand(const glm::vec3& p) {
    if (!valid) {
      bmin = bmax = p;
      valid = true;
    } else {
      bmin = glm::min(bmin, p);
      bmax = glm::max(bmax, p);
    }
    center = 0.5f * (bmin + bmax);
  }
};

#endif /* CORE_GL_INTERACTION_RAY_H_ */
