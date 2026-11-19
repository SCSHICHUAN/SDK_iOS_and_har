//
// created by stan 2026-11-18
// animator.cpp — 每帧 CPU 插值 → 骨矩阵表
//

#include "animator.h"

#include <cmath>

#include <assimp/scene.h>

namespace {

glm::mat4 AiToGlmMat4(const aiMatrix4x4& m) {
  return glm::mat4(m.a1, m.b1, m.c1, m.d1, m.a2, m.b2, m.c2, m.d2, m.a3, m.b3, m.c3, m.d3,
                   m.a4, m.b4, m.c4, m.d4);
}

}  // namespace

Animator::Animator(Animation* animation) : currentAnimation_(animation) {}

void Animator::playAnimation(Animation* animation, bool resetTime) {
  currentAnimation_ = animation;
  if (resetTime) currentTime_ = 0.0f;
}

void Animator::updateAnimation(float dt) {
  if (!currentAnimation_ || !currentAnimation_->getScene()) return;

  if (!playing_) dt = 0.0f;
  if (dt > 0.0f) {
    currentTime_ += static_cast<float>(currentAnimation_->getTicksPerSecond()) * dt;
    const float dur = currentAnimation_->getDuration();
    if (looping_) {
      if (dur > 0.0f) currentTime_ = std::fmod(currentTime_, dur);
    } else if (currentTime_ > dur) {
      currentTime_ = dur;
    }
  }

  // 每帧清空再沿骨架树重算：产出 globalBoneId → skinMatrix
  finalBoneMatrices_.clear();
  calculateBoneTransform(currentAnimation_->getScene()->mRootNode, glm::mat4(1.0f));
}

void Animator::calculateBoneTransform(const aiNode* node, const glm::mat4& parentTransform) {
  if (!node || !currentAnimation_) return;

  const std::string nodeName = node->mName.data;
  glm::mat4 nodeTransform = AiToGlmMat4(node->mTransformation);

  // 有通道：用插值后的局部矩阵；无通道：用绑定姿势节点变换
  if (Bone* bone = currentAnimation_->findBone(nodeName)) {
    bone->update(currentTime_);
    nodeTransform = bone->getLocalTransform();
  }

  const glm::mat4 globalTransformation = parentTransform * nodeTransform;

  auto& map = currentAnimation_->getBoneInfoMap();
  auto it = map.find(nodeName);
  if (it != map.end()) {
    // skin = globalInverse * global * invBind
    finalBoneMatrices_[it->second.id] =
        currentAnimation_->getGlobalInverseTransform() * globalTransformation * it->second.offset;
  }

  for (unsigned i = 0; i < node->mNumChildren; ++i) {
    if (node->mChildren[i]) {
      calculateBoneTransform(node->mChildren[i], globalTransformation);
    }
  }
}

bool Animator::isFinished() const {
  if (!currentAnimation_) return true;
  if (looping_) return false;
  return currentTime_ >= currentAnimation_->getDuration();
}
