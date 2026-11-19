//
// pick.cpp — prepare AABB；射线点选最近零件
//

#include "pick.h"

#include "explode.h"
#include "model.h"

void Pick::prepare(const Model& model) {
  clear();
  if (!model.loaded()) return;

  const auto& meshes = model.meshes;
  bounds_.resize(meshes.size());
  for (size_t i = 0; i < meshes.size(); ++i) {
    PartBounds& b = bounds_[i];
    for (const auto& v : meshes[i].vertices) {
      b.expand(v.Position);
    }
  }
}

PickHit Pick::pick(const Ray& worldRay, const glm::mat4& modelMatrix,
                   const Explode* explode) const {
  PickHit best;
  best.distance = std::numeric_limits<float>::max();

  const glm::mat4 invModel = glm::inverse(modelMatrix);
  Ray local;
  const glm::vec3 o = glm::vec3(invModel * glm::vec4(worldRay.origin, 1.0f));
  glm::vec3 d = glm::vec3(invModel * glm::vec4(worldRay.direction, 0.0f));
  const float dlen = glm::length(d);
  if (dlen < 1e-8f) return best;
  d /= dlen;
  local.origin = o;
  local.direction = d;

  for (int i = 0; i < static_cast<int>(bounds_.size()); ++i) {
    const PartBounds& src = bounds_[i];
    if (!src.valid) continue;
    glm::vec3 bmin = src.bmin;
    glm::vec3 bmax = src.bmax;
    if (explode && explode->prepared()) {
      const glm::vec3 off = explode->offset(i);
      bmin += off;
      bmax += off;
    }
    float t = 0.0f;
    if (!local.intersectAABB(bmin, bmax, t)) continue;
    if (t < best.distance) {
      best.distance = t;
      best.meshIndex = i;
      best.point = worldRay.origin + worldRay.direction * t;
    }
  }
  if (!best.hit()) best.distance = 0.0f;
  return best;
}
