//
// explode.cpp — prepare：扫 Model mesh 顶点 → 零件方向
//

#include "explode.h"

#include "model.h"

void Explode::prepare(const Model& model) {
  clear();
  if (!model.loaded()) return;

  const auto& meshes = model.meshes;
  parts_.resize(meshes.size());

  glm::vec3 modelMin(1e9f), modelMax(-1e9f);
  bool any = false;
  std::vector<PartBounds> bounds(meshes.size());

  for (size_t i = 0; i < meshes.size(); ++i) {
    PartBounds& b = bounds[i];
    for (const auto& v : meshes[i].vertices) {
      b.expand(v.Position);
      modelMin = glm::min(modelMin, v.Position);
      modelMax = glm::max(modelMax, v.Position);
      any = true;
    }
  }
  if (!any) {
    parts_.clear();
    return;
  }

  const glm::vec3 modelCenter = 0.5f * (modelMin + modelMax);
  modelRadius_ = std::max(glm::length(modelMax - modelMin) * 0.5f, 1e-3f);

  for (size_t i = 0; i < meshes.size(); ++i) {
    Part& p = parts_[i];
    if (!bounds[i].valid) {
      p.dir = glm::vec3(0.0f, 1.0f, 0.0f);
      p.dist = 0.0f;
      continue;
    }
    glm::vec3 d = bounds[i].center - modelCenter;
    const float len = glm::length(d);
    if (len < 1e-5f) {
      // 与中心重合：沿 Y 略偏，避免零向量
      d = glm::vec3(0.0f, 1.0f, 0.0f);
      p.dir = d;
    } else {
      p.dir = d / len;
    }
    // 完全爆炸位移 ≈ 半径；靠近中心的件拉得少一点
    p.dist = modelRadius_ * (0.35f + 0.65f * std::min(len / modelRadius_, 1.0f));
  }
}
