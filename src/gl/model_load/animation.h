//
// created by stan 2026-11-18
// animation.h — Animation：加载动画
//
// 从 Assimp 某一条 mAnimations 拷贝关键帧（pos/rot/scale），按骨名挂到 boneInfoMap。
// 只存 clip 数据，不建 VAO、不推进播放头；每帧播放见 animator.h。
//

#ifndef CORE_GL_ANIMATION_H_
#define CORE_GL_ANIMATION_H_

#include <map>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

struct aiNodeAnim;
struct aiScene;

/** 骨名 → 着色器索引 id + 逆绑定矩阵 offset（蒙皮用） */
struct BoneInfo {
  int id = 0;
  glm::mat4 offset{1.0f};
};

struct KeyPosition {
  glm::vec3 position{0.0f};
  float timeStamp = 0.0f;  // 单位：格(tick)
};

struct KeyRotation {
  glm::quat orientation{1.0f, 0.0f, 0.0f, 0.0f};
  float timeStamp = 0.0f;
};

struct KeyScale {
  glm::vec3 scale{1.0f};
  float timeStamp = 0.0f;
};

/** 一根骨在某条 clip 上的关键帧通道 */
class Bone {
 public:
  /**
   * 功能：从 Assimp 通道拷贝 pos/rot/scale 关键帧。
   * 参数：name — 骨名；id — 全局骨索引；channel — Assimp 通道
   * 输入：channel 有效
   * 返回：无
   */
  Bone(const std::string& name, int id, const aiNodeAnim* channel);

  /**
   * 功能：按播放头插值，得到本骨这一瞬间的局部矩阵 T*R*S。
   * 参数：animationTime — 当前格(tick)
   * 输入：animationTime
   * 返回：无；结果 getLocalTransform()
   */
  void update(float animationTime);

  /**
   * 功能：取最近一次 update 的局部矩阵。
   * 参数：无 / 输入：无
   * 返回：4x4 局部变换
   */
  glm::mat4 getLocalTransform() const { return localTransform_; }

  /** 功能：骨名。参数/输入：无。返回：名字引用 */
  const std::string& getBoneName() const { return boneName_; }

  /** 功能：全局骨 id。参数/输入：无。返回：id */
  int getBoneID() const { return boneID_; }

 private:
  int getPositionIndex(float animationTime) const;
  int getRotationIndex(float animationTime) const;
  int getScaleIndex(float animationTime) const;
  static float getScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime);
  glm::mat4 interpolatePosition(float animationTime) const;
  glm::mat4 interpolateRotation(float animationTime) const;
  glm::mat4 interpolateScaling(float animationTime) const;

  std::vector<KeyPosition> positions_;
  std::vector<KeyRotation> rotations_;
  std::vector<KeyScale> scales_;
  int boneID_ = 0;
  std::string boneName_;
  glm::mat4 localTransform_{1.0f};
};

/**
 * 一条动画 clip（Assimp mAnimations[i]）。
 * 构造须在 Model 填完 boneInfoMap 之后。
 */
class Animation {
 public:
  /**
   * 功能：从 scene 第 animationIndex 条动画创建 clip，按骨名匹配通道。
   * 参数：
   *   name — 调试用名字
   *   scene — Assimp 场景（寿命由 Model::Importer 保证）
   *   animationIndex — mAnimations 下标
   *   boneInfoMap / boneCount — 与顶点共用的骨架表
   * 输入：scene 有效且骨表已填
   * 返回：无
   */
  /**
   * cancelRootTransform — FBX 常需 inv(root) 把骨架拉回模型空间；
   * Sketchfab glTF 的 root 是 Z→Y 摆正，若再取逆会头朝下/轮子错位，应传 false。
   */
  Animation(const std::string& name, const aiScene* scene, int animationIndex,
            const std::map<std::string, BoneInfo>& boneInfoMap, int boneCount,
            bool cancelRootTransform = true);

  /**
   * 功能：按骨名查找本 clip 内的通道。
   * 参数：name — 骨名
   * 输入：name
   * 返回：Bone*；无匹配 → nullptr
   */
  Bone* findBone(const std::string& name);

  /** 功能：骨架表。参数/输入：无。返回：可写/只读 map */
  std::map<std::string, BoneInfo>& getBoneInfoMap() { return boneInfoMap_; }
  const std::map<std::string, BoneInfo>& getBoneInfoMap() const { return boneInfoMap_; }

  /** 功能：骨数量。返回：boneCount_ */
  int getBoneCount() const { return boneCount_; }

  /** 功能：动画总长。返回：格(tick) */
  float getDuration() const { return duration_; }

  /** 功能：时间基。返回：tick/秒 */
  int getTicksPerSecond() const { return ticksPerSecond_; }

  /** 功能：源场景（算骨树用）。返回：aiScene* */
  const aiScene* getScene() const { return scene_; }

  /** 功能：根逆变换。返回：globalInverse */
  const glm::mat4& getGlobalInverseTransform() const { return globalInverseTransform_; }

  /** 功能：在文件中的下标。返回：index */
  int getAnimationIndex() const { return animationIndex_; }

  /** 功能：Assimp 原始名字。返回：字符串 */
  const std::string& getSourceName() const { return sourceName_; }

  /** 功能：成功匹配到骨表的通道数。返回：count */
  int getMatchedChannels() const { return static_cast<int>(bones_.size()); }

 private:
  float duration_ = 0.0f;
  int ticksPerSecond_ = 25;
  std::vector<Bone> bones_;
  std::map<std::string, BoneInfo> boneInfoMap_;
  int boneCount_ = 0;
  std::string animationName_;
  std::string sourceName_;
  const aiScene* scene_ = nullptr;
  glm::mat4 globalInverseTransform_{1.0f};
  int animationIndex_ = 0;
};

#endif /* CORE_GL_ANIMATION_H_ */
