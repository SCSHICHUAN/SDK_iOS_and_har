//
// created by stan 2026-11-18
// animation.cpp — clip 关键帧加载（Bone / Animation）
//

#include "animation.h"

#include <cmath>
#include <iostream>

#include <assimp/anim.h>
#include <assimp/scene.h>

namespace {

glm::mat4 AiToGlmMat4(const aiMatrix4x4& m) {
  return glm::mat4(m.a1, m.b1, m.c1, m.d1, m.a2, m.b2, m.c2, m.d2, m.a3, m.b3, m.c3, m.d3,
                   m.a4, m.b4, m.c4, m.d4);
}

}  // namespace

// —— Bone：一条通道的关键帧 ——
Bone::Bone(const std::string& name, int id, const aiNodeAnim* channel)
    : boneID_(id), boneName_(name), localTransform_(1.0f) {
  if (!channel) return;

  for (unsigned i = 0; i < channel->mNumPositionKeys; ++i) {
    const aiVectorKey& k = channel->mPositionKeys[i];
    KeyPosition data;
    data.position = glm::vec3(k.mValue.x, k.mValue.y, k.mValue.z);
    data.timeStamp = static_cast<float>(k.mTime);
    positions_.push_back(data);
  }
  for (unsigned i = 0; i < channel->mNumRotationKeys; ++i) {
    const aiQuatKey& k = channel->mRotationKeys[i];
    KeyRotation data;
    data.orientation = glm::quat(k.mValue.w, k.mValue.x, k.mValue.y, k.mValue.z);
    data.timeStamp = static_cast<float>(k.mTime);
    rotations_.push_back(data);
  }
  for (unsigned i = 0; i < channel->mNumScalingKeys; ++i) {
    const aiVectorKey& k = channel->mScalingKeys[i];
    KeyScale data;
    data.scale = glm::vec3(k.mValue.x, k.mValue.y, k.mValue.z);
    data.timeStamp = static_cast<float>(k.mTime);
    scales_.push_back(data);
  }
}

void Bone::update(float animationTime) {
  localTransform_ = interpolatePosition(animationTime) * interpolateRotation(animationTime) *
                    interpolateScaling(animationTime);
}

int Bone::getPositionIndex(float animationTime) const {
  for (int i = 0; i + 1 < static_cast<int>(positions_.size()); ++i) {
    if (animationTime < positions_[i + 1].timeStamp) return i;
  }
  return 0;
}

int Bone::getRotationIndex(float animationTime) const {
  for (int i = 0; i + 1 < static_cast<int>(rotations_.size()); ++i) {
    if (animationTime < rotations_[i + 1].timeStamp) return i;
  }
  return 0;
}

int Bone::getScaleIndex(float animationTime) const {
  for (int i = 0; i + 1 < static_cast<int>(scales_.size()); ++i) {
    if (animationTime < scales_[i + 1].timeStamp) return i;
  }
  return 0;
}

float Bone::getScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime) {
  const float mid = animationTime - lastTimeStamp;
  const float diff = nextTimeStamp - lastTimeStamp;
  return (diff > 1e-8f) ? (mid / diff) : 0.0f;
}

glm::mat4 Bone::interpolatePosition(float animationTime) const {
  if (positions_.empty()) return glm::mat4(1.0f);
  if (positions_.size() == 1) return glm::translate(glm::mat4(1.0f), positions_[0].position);
  if (animationTime >= positions_.back().timeStamp) {
    return glm::translate(glm::mat4(1.0f), positions_.back().position);
  }
  const int i0 = getPositionIndex(animationTime);
  const int i1 = i0 + 1;
  const float t =
      getScaleFactor(positions_[i0].timeStamp, positions_[i1].timeStamp, animationTime);
  const glm::vec3 p = glm::mix(positions_[i0].position, positions_[i1].position, t);
  return glm::translate(glm::mat4(1.0f), p);
}

glm::mat4 Bone::interpolateRotation(float animationTime) const {
  if (rotations_.empty()) return glm::mat4(1.0f);
  if (rotations_.size() == 1) {
    return glm::mat4_cast(glm::normalize(rotations_[0].orientation));
  }
  if (animationTime >= rotations_.back().timeStamp) {
    return glm::mat4_cast(glm::normalize(rotations_.back().orientation));
  }
  const int i0 = getRotationIndex(animationTime);
  const int i1 = i0 + 1;
  const float t =
      getScaleFactor(rotations_[i0].timeStamp, rotations_[i1].timeStamp, animationTime);
  glm::quat q = glm::slerp(rotations_[i0].orientation, rotations_[i1].orientation, t);
  return glm::mat4_cast(glm::normalize(q));
}

glm::mat4 Bone::interpolateScaling(float animationTime) const {
  if (scales_.empty()) return glm::mat4(1.0f);
  if (scales_.size() == 1) return glm::scale(glm::mat4(1.0f), scales_[0].scale);
  if (animationTime >= scales_.back().timeStamp) {
    return glm::scale(glm::mat4(1.0f), scales_.back().scale);
  }
  const int i0 = getScaleIndex(animationTime);
  const int i1 = i0 + 1;
  const float t = getScaleFactor(scales_[i0].timeStamp, scales_[i1].timeStamp, animationTime);
  const glm::vec3 s = glm::mix(scales_[i0].scale, scales_[i1].scale, t);
  return glm::scale(glm::mat4(1.0f), s);
}

// —— Animation：一条 clip ——
Animation::Animation(const std::string& name, const aiScene* scene, int animationIndex,
                     const std::map<std::string, BoneInfo>& boneInfoMap, int boneCount,
                     bool cancelRootTransform)
    : boneInfoMap_(boneInfoMap),
      boneCount_(boneCount),
      animationName_(name),
      scene_(scene),
      animationIndex_(animationIndex) {
  if (!scene || scene->mNumAnimations == 0) {
    duration_ = 0.0f;
    ticksPerSecond_ = 25;
    sourceName_ = "<no-anim>";
    return;
  }
  if (animationIndex_ < 0 ||
      animationIndex_ >= static_cast<int>(scene->mNumAnimations)) {
    animationIndex_ = 0;
  }

  aiAnimation* animation = scene->mAnimations[animationIndex_];
  sourceName_ = (animation && animation->mName.length > 0) ? animation->mName.C_Str() : "<unnamed>";
  duration_ = static_cast<float>(animation->mDuration);
  ticksPerSecond_ = animation->mTicksPerSecond != 0.0
                        ? static_cast<int>(animation->mTicksPerSecond)
                        : 25;

  // FBX：inv(root) 纠正厂商根变换；glTF/Sketchfab：root 含 Z→Y，取消会整车躺倒
  if (cancelRootTransform && scene->mRootNode) {
    globalInverseTransform_ = glm::inverse(AiToGlmMat4(scene->mRootNode->mTransformation));
  } else {
    globalInverseTransform_ = glm::mat4(1.0f);
  }

  // 只挂 boneInfoMap 里有的通道（和顶点骨 ID 同一套名字）
  for (unsigned i = 0; i < animation->mNumChannels; ++i) {
    aiNodeAnim* channel = animation->mChannels[i];
    const std::string boneName = channel->mNodeName.data;
    auto it = boneInfoMap_.find(boneName);
    if (it == boneInfoMap_.end()) continue;
    bones_.emplace_back(boneName, it->second.id, channel);
  }
}

Bone* Animation::findBone(const std::string& name) {
  for (auto& bone : bones_) {
    if (bone.getBoneName() == name) return &bone;
  }
  return nullptr;
}
