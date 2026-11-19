//
// created by stan 2026-11-18
// animator.h — Animator：播放动画
//
// 拿 Animation 里已加载的关键帧，每帧推进播放头、CPU 插值，产出骨矩阵表。
// 不加载数据、不生产 VAO；矩阵交给 Mesh::Draw 上传。
//

#ifndef CORE_GL_ANIMATOR_H_
#define CORE_GL_ANIMATOR_H_

#include "animation.h"

#include <map>

#include <glm/glm.hpp>

struct aiNode;

class Animator {
 public:
  /**
   * 功能：创建播放器，可选绑定一条初始 clip。
   * 参数：animation — 可为 nullptr（稍后 playAnimation）
   * 输入：animation（不取得所有权，寿命由 Model 保证）
   * 返回：无
   */
  explicit Animator(Animation* animation = nullptr);

  /**
   * 功能：推进播放头并重算全部骨矩阵（约每帧调用一次）。
   * 参数：dt — 秒；传 0 不推进时间，仍重算矩阵（暂停/叠表情时有用）
   * 输入：dt；须已 play 过有效 Animation
   * 返回：无；结果在 getFinalBoneMatrices()
   */
  void updateAnimation(float dt);

  /**
   * 功能：切换当前播放的 clip（用户点选某条动画）。
   * 参数：
   *   animation — 目标 clip，不可为 nullptr
   *   resetTime — true 播放头归零；false 保持当前格
   * 输入：animation, resetTime
   * 返回：无
   */
  void playAnimation(Animation* animation, bool resetTime = true);

  /**
   * 功能：设置是否循环。
   * 参数：enabled — true 循环；false 播完停在末帧
   * 输入：enabled
   * 返回：无
   */
  void setLooping(bool enabled) { looping_ = enabled; }

  /**
   * 功能：查询是否循环。
   * 参数：无
   * 输入：无
   * 返回：true = 循环中
   */
  bool isLooping() const { return looping_; }

  /**
   * 功能：是否推进播放头。false 时仍每帧重算绑定姿势矩阵（车门动画默认停住）。
   */
  void setPlaying(bool enabled) { playing_ = enabled; }
  bool isPlaying() const { return playing_; }

  /**
   * 功能：非循环时是否已播完。
   * 参数：无
   * 输入：无
   * 返回：true = 已结束（无动画或非循环且到时长）
   */
  bool isFinished() const;

  /**
   * 功能：当前绑定的 clip。
   * 参数：无
   * 输入：无
   * 返回：Animation*；未绑定时为 nullptr
   */
  Animation* currentAnimation() const { return currentAnimation_; }

  /**
   * 功能：取本帧算好的蒙皮矩阵表（globalBoneId → mat4）。
   * 参数：无
   * 输入：须先 updateAnimation
   * 返回：可写/只读引用，供 Mesh::Draw 上传
   */
  std::map<int, glm::mat4>& getFinalBoneMatrices() { return finalBoneMatrices_; }
  const std::map<int, glm::mat4>& getFinalBoneMatrices() const { return finalBoneMatrices_; }

 private:
  /** 沿 Assimp 节点树递归：父全局 × 本骨局部 → skin 矩阵写入 finalBoneMatrices_ */
  void calculateBoneTransform(const aiNode* node, const glm::mat4& parentTransform);

  Animation* currentAnimation_ = nullptr;
  float currentTime_ = 0.0f;  // 播放头，单位：格(tick)
  std::map<int, glm::mat4> finalBoneMatrices_;
  bool looping_ = true;
  bool playing_ = true;
};

#endif /* CORE_GL_ANIMATOR_H_ */
