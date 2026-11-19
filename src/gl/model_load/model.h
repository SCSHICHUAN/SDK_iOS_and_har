//
// model.h — Model：加载数据
// created by stan 2026-11-18
//
// 从文件/内存读 Assimp 场景，拆成两路：
//   顶点/索引/贴图/骨 ID → 交给 Mesh（由 Mesh 生产 VAO）
//   动画 clip            → 交给 Animation（只存关键帧）
// Draw：遍历各 Mesh，换 VAO 画；可选带 Animator 的骨矩阵。
//

#ifndef CORE_GL_MODEL_H_
#define CORE_GL_MODEL_H_

#include "animation.h"
#include "mesh.h"
#include "shader.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>

namespace Assimp {
class Importer;
}
struct aiScene;
struct aiNode;
struct aiMesh;
struct aiMaterial;

class Model {
 public:
  std::vector<Texture> textures_loaded;
  std::vector<Mesh> meshes;
  std::string directory;
  std::map<std::string, BoneInfo> boneInfoMap;
  int boneCount = 0;
  std::vector<Animation*> animations;  // Model 拥有；destroy 时 delete

  /**
   * 功能：构造并立即按路径加载。
   * 参数：path — 模型文件路径
   * 输入：path；须 GLES Context current（会建 VAO）
   * 返回：无（失败则 loaded()==false）
   */
  // 默认/带参构造与析构均在 .cpp：unique_ptr<Assimp::Importer> 需要完整类型
  Model();
  explicit Model(const std::string& path);
  ~Model();

  Model(const Model&) = delete;
  Model& operator=(const Model&) = delete;

  /**
   * 功能：从路径加载；顶点→Mesh(VAO)，动画→Animation。
   * 参数：path — UTF-8 路径
   * 输入：path；Context current
   * 返回：true 成功；false Assimp/解析失败
   */
  bool loadModel(const std::string& path);

  /**
   * 功能：从内存字节加载（包内资源）。
   * 参数：data/size — 字节；hint — 扩展名如 ".glb"
   * 输入：data[0..size)；Context current
   * 返回：true 成功；false 失败
   */
  bool loadModelFromMemory(const void* data, size_t size, const char* hint = ".obj");

  /**
   * 功能：提交所有 Mesh（绑 VAO + 可选上传骨矩阵）。
   * 参数：shader — 当前 program；boneMatrices — Animator 产出，可 nullptr
   * 输入：shader 已 use；矩阵表若有须与骨 ID 对应
   * 返回：无
   */
  void Draw(const Shader& shader,
            const std::map<int, glm::mat4>* boneMatrices = nullptr) const;

  /**
   * 功能：释放 Mesh/贴图/Animation/Importer。
   * 参数/输入：无
   * 返回：无
   */
  void destroy();

  /** 功能：AABB 中心。返回：center_ */
  const glm::vec3& center() const { return center_; }

  /** 功能：包围球半径。返回：radius_ */
  float radius() const { return radius_; }

  /**
   * 功能：建议世界缩放（与灯立方体同一套单位）。
   * FBX 游戏单位 → 0.018（对齐 AR）；其它 → 1。
   */
  float preferredWorldScale() const { return preferredWorldScale_; }

  /**
   * 功能：资源「正面」绕 Y 校正（弧度）。
   * Assimp 无统一 front：glTF 车（SU7/Benz，含蒙皮车）常背对 → π；
   * 蒙皮人模 / FBX（狼、whiteMan、blackMan）→ 0。
   * 进场结束角 = frontYawOffset + 45°。
   */
  float preferredFrontYawOffset() const { return preferredFrontYawOffset_; }

  /**
   * 功能：是否车辆类资源（车门/引擎盖动画或 carpaint 材质）。
   * 此类资源默认停在绑定姿势，不自动循环播放开闭动画。
   */
  bool isVehicleAsset() const { return isVehicleAsset_; }

  /** 功能：是否按 FBX 资源加载。返回：isFbxAsset_ */
  bool isFbxAsset() const { return isFbxAsset_; }

  /** 功能：是否已有网格。返回：!meshes.empty() */
  bool loaded() const { return !meshes.empty(); }

  /** 功能：clip 条数。返回：animations.size() */
  int getAnimationCount() const { return static_cast<int>(animations.size()); }

  /**
   * 功能：取第 index 条动画。
   * 参数：index — [0, count)
   * 输入：index
   * 返回：Animation*；越界 → nullptr
   */
  Animation* getAnimation(int index) const {
    if (index < 0 || index >= static_cast<int>(animations.size())) return nullptr;
    return animations[index];
  }

  /**
   * 功能：Assimp 场景指针（Importer 保活，供 Animator 走骨树）。
   * 参数/输入：无
   * 返回：aiScene*；未加载可为 nullptr
   */
  const aiScene* scene() const { return scene_; }

 private:
  glm::vec3 center_{0.0f};
  float radius_{1.0f};
  float preferredWorldScale_{1.0f};
  float preferredFrontYawOffset_{0.0f};
  bool isFbxAsset_ = false;
  bool isVehicleAsset_ = false;
  GLuint fallbackTex_ = 0;
  std::unique_ptr<Assimp::Importer> importer_;
  const aiScene* scene_ = nullptr;

  bool finishLoadFromScene(const std::string& directoryHint, bool isFbx);
  // parentGlobal：根到当前节点的累积变换。静态 mesh（无骨）会 bake 进顶点，否则 Sketchfab/glTF 的 Y↔Z 摆正丢失。
  void parseNode(const aiNode* node, const aiScene* scene, const glm::mat4& parentGlobal);
  Mesh processMesh(const aiMesh* mesh, const aiScene* scene, const glm::mat4& nodeGlobal);
  void recalculateFbxBoneOffsets();  // Assimp FBX mOffsetMatrix 常错，对齐 AR
  void loadAnimations(const aiScene* scene);
  void detectVehicleAsset(const aiScene* scene);
  void computeBounds(const std::map<int, glm::mat4>* bindPoseBones = nullptr);

  std::vector<Texture> loadMaterialTextures(const aiMaterial* mat, int type,
                                            const std::string& typeName);
  unsigned int textureFromFile(const char* path, const std::string& directory);
  unsigned int makeSolidTexture(unsigned char r, unsigned char g, unsigned char b);
};

#endif /* CORE_GL_MODEL_H_ */
