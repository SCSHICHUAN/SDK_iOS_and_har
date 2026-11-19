//
// pick.h — 点击选取
//
// 射线 vs 各零件 AABB，取最近命中；维护当前选中 meshIndex。
// 依赖 Ray / PartBounds（见 ray.h）。高亮绘制由 Render 读 selected()。
//

#ifndef CORE_GL_INTERACTION_PICK_H_
#define CORE_GL_INTERACTION_PICK_H_

#include "ray.h"

#include <limits>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>

class Model;
class Explode;

struct PickHit {
  int meshIndex = -1;
  float distance = 0.0f;
  glm::vec3 point{0.0f};
  bool hit() const { return meshIndex >= 0; }
};

/**
 * 点选状态机。
 * prepare(Model) → pick/select(Ray, modelMatrix [, explode])。
 */
class Pick {
 public:
  int selected() const { return selected_; }
  void setSelected(int meshIndex) { selected_ = meshIndex; }
  void clearSelection() { selected_ = -1; }
  bool prepared() const { return !bounds_.empty(); }
  int partCount() const { return static_cast<int>(bounds_.size()); }

  const PartBounds& bounds(int meshIndex) const {
    static const PartBounds kEmpty{};
    if (meshIndex < 0 || meshIndex >= static_cast<int>(bounds_.size())) return kEmpty;
    return bounds_[meshIndex];
  }

  /** 功能：缓存各 mesh AABB（模型空间顶点） */
  void prepare(const Model& model);

  /**
   * 功能：射线点选，不改 selected。
   * 参数：ray — 世界空间；modelMatrix — 与 Draw 相同（含缩放/arcball，不含 explode）；
   *       explode — 可选，命中盒随爆炸位移
   * 返回：最近命中；未中 meshIndex=-1
   */
  PickHit pick(const Ray& worldRay, const glm::mat4& modelMatrix,
               const Explode* explode = nullptr) const;

  /** 点选并写入 selected_；未中则清选取。返回是否命中 */
  bool select(const Ray& worldRay, const glm::mat4& modelMatrix,
              const Explode* explode = nullptr) {
    const PickHit h = pick(worldRay, modelMatrix, explode);
    selected_ = h.meshIndex;
    return h.hit();
  }

  void clear() {
    selected_ = -1;
    bounds_.clear();
  }

 private:
  int selected_ = -1;
  std::vector<PartBounds> bounds_;
};

#endif /* CORE_GL_INTERACTION_PICK_H_ */
