//
// created by stan 2026-11-18
// model.cpp — ParseScene → UploadMeshes(VAO) → LoadAnimations
//

#include "model.h"

#include "animator.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <utility>
#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace {

unsigned int uploadRGBATexture(const unsigned char* rgba, int w, int h) {
  if (!rgba || w <= 0 || h <= 0) return 0;
  unsigned int tex = 0;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
  glGenerateMipmap(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glBindTexture(GL_TEXTURE_2D, 0);
  return tex;
}

unsigned int loadTexture2D(const std::string& path) {
  int w = 0, h = 0, comp = 0;
  // 与 AR 一致：只靠 aiProcess_FlipUVs，不再 stbi 竖翻（否则双翻 → UV 采到 Body 图集黑底）
  stbi_set_flip_vertically_on_load(false);
  unsigned char* data = stbi_load(path.c_str(), &w, &h, &comp, 4);
  if (!data) return 0;
  const unsigned int tex = uploadRGBATexture(data, w, h);
  stbi_image_free(data);
  return tex;
}

/** GLB/glTF 内嵌贴图（路径常为 "*0"）；失败返回 0 */
unsigned int loadEmbeddedTexture(const aiScene* scene, const char* path) {
  if (!scene || !path || !path[0]) return 0;
  const aiTexture* emb = scene->GetEmbeddedTexture(path);
  if (!emb) return 0;

  int w = 0, h = 0, comp = 0;
  unsigned char* data = nullptr;
  bool freeStbi = true;
  if (emb->mHeight == 0) {
    data = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(emb->pcData),
                                 static_cast<int>(emb->mWidth), &w, &h, &comp, 4);
  } else {
    w = static_cast<int>(emb->mWidth);
    h = static_cast<int>(emb->mHeight);
    data = static_cast<unsigned char*>(std::malloc(static_cast<size_t>(w) * static_cast<size_t>(h) * 4));
    freeStbi = false;
    if (data) {
      for (int i = 0; i < w * h; ++i) {
        const aiTexel& t = emb->pcData[i];
        data[i * 4 + 0] = t.r;
        data[i * 4 + 1] = t.g;
        data[i * 4 + 2] = t.b;
        data[i * 4 + 3] = t.a;
      }
    }
  }
  if (!data) return 0;
  const unsigned int tex = uploadRGBATexture(data, w, h);
  if (freeStbi) stbi_image_free(data);
  else std::free(data);
  return tex;
}

/** FBX 常带 textures\xxx.jpg 或 Windows 绝对路径；多候选查找。 */
std::string resolveTexturePath(const std::string& rawPath, const std::string& directory) {
  std::string filename = rawPath;
  std::replace(filename.begin(), filename.end(), '\\', '/');

  auto basename = [](const std::string& p) -> std::string {
    const auto slash = p.find_last_of('/');
    return (slash == std::string::npos) ? p : p.substr(slash + 1);
  };
  const std::string base = basename(filename);

  std::vector<std::string> candidates;
  candidates.push_back(directory + "/" + filename);
  candidates.push_back(filename);  // 偶发已是绝对/相对可用路径
  if (!base.empty()) {
    candidates.push_back(directory + "/" + base);
    candidates.push_back(directory + "/textures/" + base);
    candidates.push_back(directory + "/TEXTURES/" + base);
  }

  for (const auto& cand : candidates) {
    FILE* f = std::fopen(cand.c_str(), "rb");
    if (f) {
      std::fclose(f);
      return cand;
    }
  }
  return {};
}

glm::mat4 AiToGlmMat4(const aiMatrix4x4& m) {
  return glm::mat4(m.a1, m.b1, m.c1, m.d1, m.a2, m.b2, m.c2, m.d2, m.a3, m.b3, m.c3, m.d3,
                   m.a4, m.b4, m.c4, m.d4);
}

// 对齐 AR：勿 JoinIdenticalVertices（易焊死不同 UV 的顶点 → 图集黑底/条纹）
constexpr unsigned kAssimpFlags =
    aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs |
    aiProcess_CalcTangentSpace | aiProcess_SplitByBoneCount |
    aiProcess_LimitBoneWeights;

}  // namespace

Model::Model() = default;

Model::Model(const std::string& path) { loadModel(path); }

Model::~Model() { destroy(); }

unsigned int Model::makeSolidTexture(unsigned char r, unsigned char g, unsigned char b) {
  unsigned char px[4] = {r, g, b, 255};
  unsigned int tex = 0;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glBindTexture(GL_TEXTURE_2D, 0);
  return tex;
}

unsigned int Model::textureFromFile(const char* path, const std::string& dir) {
  if (!path || !path[0]) return 0;

  // 1) GLB 内嵌（"*0" 等）
  const std::string embedKey = std::string("embedded:") + path;
  for (const auto& t : textures_loaded) {
    if (t.path == embedKey) return t.id;
  }
  if (unsigned int embId = loadEmbeddedTexture(scene_, path)) {
    Texture tex;
    tex.id = embId;
    tex.type = "texture_diffuse";
    tex.path = embedKey;
    textures_loaded.push_back(tex);
    return embId;
  }

  // 2) 磁盘文件
  const std::string resolved = resolveTexturePath(path, dir);
  if (resolved.empty()) {
    std::cerr << "texture miss: \"" << path << "\" dir=" << dir << "\n";
    return 0;
  }
  for (const auto& t : textures_loaded) {
    if (t.path == resolved) return t.id;
  }
  unsigned int id = loadTexture2D(resolved);
  if (!id) {
    std::cerr << "stb_image failed: " << resolved << " (" << stbi_failure_reason() << ")\n";
    return 0;
  }
  Texture tex;
  tex.id = id;
  tex.type = "texture_diffuse";
  tex.path = resolved;
  textures_loaded.push_back(tex);
  return id;
}

std::vector<Texture> Model::loadMaterialTextures(const aiMaterial* mat, int type,
                                                 const std::string& typeName) {
  std::vector<Texture> textures;
  if (!mat) return textures;
  const aiTextureType aiType = static_cast<aiTextureType>(type);
  const unsigned count = mat->GetTextureCount(aiType);
  for (unsigned i = 0; i < count; ++i) {
    aiString str;
    mat->GetTexture(aiType, i, &str);
    unsigned int id = textureFromFile(str.C_Str(), directory);
    if (!id) continue;
    Texture tex;
    tex.id = id;
    tex.type = typeName;
    tex.path = str.C_Str();
    // 用已解析的真实路径（含 textures/），供 Fur cutout 按文件名判断
    for (const auto& cached : textures_loaded) {
      if (cached.id == id) {
        tex.path = cached.path;
        break;
      }
    }
    textures.push_back(tex);
  }
  return textures;
}

Mesh Model::processMesh(const aiMesh* mesh, const aiScene* scene, const glm::mat4& nodeGlobal) {
  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;
  std::vector<Texture> textures;
  std::vector<int> bonePalette;
  vertices.reserve(mesh->mNumVertices);

  // 有骨骼：顶点留在 mesh 局部空间（蒙皮公式依赖）；无骨：bake 节点矩阵（SU7 摆正/0.01 缩放）
  const bool bakeNode = (mesh->mNumBones == 0);
  const glm::mat3 normalMat =
      bakeNode ? glm::mat3(glm::transpose(glm::inverse(nodeGlobal))) : glm::mat3(1.0f);

  // —— 顶点位置 / 法线 / UV（CPU）——
  for (unsigned i = 0; i < mesh->mNumVertices; ++i) {
    Vertex v;
    glm::vec3 pos(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
    if (bakeNode) {
      const glm::vec4 hp = nodeGlobal * glm::vec4(pos, 1.0f);
      v.Position = glm::vec3(hp);
    } else {
      v.Position = pos;
    }
    if (mesh->HasNormals()) {
      glm::vec3 n(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
      if (bakeNode) {
        n = glm::normalize(normalMat * n);
      }
      v.Normal = n;
    }
    if (mesh->mTextureCoords[0]) {
      v.TexCoords = {mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y};
    }
    vertices.push_back(v);
  }

  for (unsigned f = 0; f < mesh->mNumFaces; ++f) {
    const aiFace& face = mesh->mFaces[f];
    for (unsigned j = 0; j < face.mNumIndices; ++j) {
      indices.push_back(face.mIndices[j]);
    }
  }

  // —— 骨骼权重：写入顶点 BoneIDs/Weights；同时填全局 boneInfoMap ——
  // 顶点用「本 mesh 局部骨槽」；palette[local]=globalId（shader 只有 100 槽）
  std::map<std::string, int> localBoneIds;
  if (mesh->mNumBones > 0) {
    for (unsigned i = 0; i < mesh->mNumBones; ++i) {
      aiBone* bone = mesh->mBones[i];
      const std::string boneName = bone->mName.data;

      if (boneInfoMap.find(boneName) == boneInfoMap.end()) {
        BoneInfo info;
        info.id = boneCount;
        info.offset = AiToGlmMat4(bone->mOffsetMatrix);
        boneInfoMap[boneName] = info;
        ++boneCount;
      }
      const int globalId = boneInfoMap[boneName].id;

      int localId = 0;
      auto lit = localBoneIds.find(boneName);
      if (lit == localBoneIds.end()) {
        localId = static_cast<int>(bonePalette.size());
        localBoneIds[boneName] = localId;
        bonePalette.push_back(globalId);
      } else {
        localId = lit->second;
      }

      for (unsigned j = 0; j < bone->mNumWeights; ++j) {
        const aiVertexWeight& w = bone->mWeights[j];
        if (w.mVertexId < vertices.size()) {
          vertices[w.mVertexId].addBoneData(localId, w.mWeight);
        }
      }
    }
  }

  glm::vec4 matColor(1.0f, 1.0f, 1.0f, 1.0f);
  bool isHairMat = false;
  if (mesh->mMaterialIndex < scene->mNumMaterials) {
    aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
    std::string matName = mat && mat->GetName().length ? mat->GetName().C_Str() : "";
    std::string meshName = mesh->mName.length ? mesh->mName.C_Str() : "";
    for (char& c : matName) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    for (char& c : meshName) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    isHairMat = (matName.find("hair") != std::string::npos ||
                 meshName.find("hair") != std::string::npos);

    // glTF：BASE_COLOR 优先。切勿把 UNKNOWN（常为 metallicRoughness）当漫反射，
    // 否则 SU7 车身会变成青白斑块。
    auto diffuse = loadMaterialTextures(mat, aiTextureType_BASE_COLOR, "texture_diffuse");
    if (diffuse.empty()) {
      diffuse = loadMaterialTextures(mat, aiTextureType_DIFFUSE, "texture_diffuse");
    }
    if (diffuse.empty()) {
      diffuse = loadMaterialTextures(mat, aiTextureType_EMISSIVE, "texture_diffuse");
    }
    // 头发：部分资源把贴图挂在 opacity / unknown
    if (diffuse.empty() && isHairMat) {
      diffuse = loadMaterialTextures(mat, aiTextureType_OPACITY, "texture_diffuse");
    }
    if (diffuse.empty() && isHairMat) {
      diffuse = loadMaterialTextures(mat, aiTextureType_UNKNOWN, "texture_diffuse");
    }
    textures.insert(textures.end(), diffuse.begin(), diffuse.end());

    aiColor4D col;
    if (AI_SUCCESS == aiGetMaterialColor(mat, AI_MATKEY_BASE_COLOR, &col) ||
        AI_SUCCESS == aiGetMaterialColor(mat, AI_MATKEY_COLOR_DIFFUSE, &col)) {
      matColor = glm::vec4(col.r, col.g, col.b, col.a);
    }
    // 已有颜色贴图时（FBX 狼等）底色常很暗，勿再乘；头发保留 factor
    if (!diffuse.empty() && !isHairMat) {
      matColor = glm::vec4(1.0f, 1.0f, 1.0f, matColor.a < 1.0f ? matColor.a : 1.0f);
    }
    // 头发：保留资源自带 factor；仅当近黑且无贴图时用深棕兜底（与 AR 一致，勿臆造发色）
    if (isHairMat && diffuse.empty() && (matColor.r + matColor.g + matColor.b) < 0.05f) {
      matColor = glm::vec4(0.20f, 0.14f, 0.10f, 1.0f);
    }
    // Benz 等：carpaint 无 albedo（glb 默认白）→ 银灰车漆
    const bool isCarPaint = (matName.find("carpaint") != std::string::npos ||
                             matName.find("car_paint") != std::string::npos);
    if (isCarPaint && diffuse.empty() && (matColor.r + matColor.g + matColor.b) > 2.7f) {
      matColor = glm::vec4(0.42f, 0.43f, 0.45f, matColor.a);
    }
  }
  // 头发无贴图时不要塞白 fallback（否则 hasDiffuse=true，黑 factor×白图仍是黑）
  if (textures.empty() && fallbackTex_ && !isHairMat) {
    Texture t;
    t.id = fallbackTex_;
    t.type = "texture_diffuse";
    textures.push_back(t);
  }

  // Mesh 构造内部 setupMesh → 产出 VAO（GPU 上传）
  Mesh out(std::move(vertices), std::move(indices), std::move(textures),
           std::move(bonePalette));
  out.materialColor = matColor;
  out.isHair = isHairMat;
  return out;
}

void Model::parseNode(const aiNode* node, const aiScene* scene, const glm::mat4& parentGlobal) {
  if (!node) return;
  const glm::mat4 global = parentGlobal * AiToGlmMat4(node->mTransformation);
  for (unsigned i = 0; i < node->mNumMeshes; ++i) {
    aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
    meshes.push_back(processMesh(mesh, scene, global));
  }
  for (unsigned i = 0; i < node->mNumChildren; ++i) {
    parseNode(node->mChildren[i], scene, global);
  }
}

void Model::loadAnimations(const aiScene* scene) {
  // 必须在 processMesh 填完 boneInfoMap 之后调用
  if (!scene || scene->mNumAnimations == 0 || boneCount <= 0) return;

  // glTF/Sketchfab：勿 cancel root（Z→Y 摆正）；FBX：需要 inv(root)
  const bool cancelRoot = isFbxAsset_;
  animations.reserve(scene->mNumAnimations);
  for (unsigned i = 0; i < scene->mNumAnimations; ++i) {
    animations.push_back(
        new Animation("Anim" + std::to_string(i), scene, static_cast<int>(i), boneInfoMap,
                      boneCount, cancelRoot));
  }
}

void Model::detectVehicleAsset(const aiScene* scene) {
  isVehicleAsset_ = false;
  if (!scene) return;
  auto containsVehicleToken = [](const std::string& s) {
    std::string l = s;
    for (char& c : l) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return l.find("carpaint") != std::string::npos || l.find("car_paint") != std::string::npos ||
           l.find("door") != std::string::npos || l.find("hood") != std::string::npos ||
           l.find("trunk") != std::string::npos || l.find("wiper") != std::string::npos ||
           l.find("tyre") != std::string::npos || l.find("tire") != std::string::npos ||
           l.find("rim_") != std::string::npos;
  };
  for (unsigned i = 0; i < scene->mNumMaterials; ++i) {
    const aiMaterial* mat = scene->mMaterials[i];
    if (!mat || !mat->GetName().length) continue;
    if (containsVehicleToken(mat->GetName().C_Str())) {
      isVehicleAsset_ = true;
      return;
    }
  }
  for (unsigned i = 0; i < scene->mNumAnimations; ++i) {
    const aiAnimation* a = scene->mAnimations[i];
    if (!a || !a->mName.length) continue;
    if (containsVehicleToken(a->mName.C_Str())) {
      isVehicleAsset_ = true;
      return;
    }
  }
}

void Model::computeBounds(const std::map<int, glm::mat4>* bindPoseBones) {
  glm::vec3 bmin(1e9f), bmax(-1e9f);
  bool any = false;
  for (const auto& mesh : meshes) {
    const bool skin = bindPoseBones && !mesh.bonePalette.empty();
    for (const auto& v : mesh.vertices) {
      glm::vec3 pos = v.Position;
      if (skin) {
        glm::vec4 skinned(0.0f);
        float wsum = 0.0f;
        for (int i = 0; i < kMaxBoneInfluence; ++i) {
          const int local = static_cast<int>(v.BoneIDs[i]);
          const float w = v.Weights[i];
          if (local < 0 || w <= 0.0f) continue;
          if (local >= static_cast<int>(mesh.bonePalette.size())) continue;
          const int globalId = mesh.bonePalette[local];
          auto it = bindPoseBones->find(globalId);
          if (it == bindPoseBones->end()) continue;
          skinned += (it->second * glm::vec4(v.Position, 1.0f)) * w;
          wsum += w;
        }
        if (wsum > 1e-6f) pos = glm::vec3(skinned);
      }
      bmin = glm::min(bmin, pos);
      bmax = glm::max(bmax, pos);
      any = true;
    }
  }
  if (!any) {
    center_ = glm::vec3(0.0f);
    radius_ = 1.0f;
    return;
  }
  center_ = 0.5f * (bmin + bmax);
  radius_ = glm::length(bmax - bmin) * 0.5f;
  if (radius_ < 1e-4f) radius_ = 1.0f;
}

void Model::recalculateFbxBoneOffsets() {
  // Assimp FBX：厂商 mOffsetMatrix 常与节点层级不一致 → 蒙皮炸开。
  // 用 bind 姿势重算 offset = inv(boneGlobal) * meshGlobal（与 AR 一致）。
  // glTF/GLB 的 inverseBindMatrices 一般是对的，不要对非 FBX 调用。
  if (!scene_ || !scene_->mRootNode || boneCount <= 0) return;

  glm::mat4 meshGlobal(1.0f);
  bool foundMesh = false;
  std::function<void(const aiNode*, const glm::mat4&)> findMesh =
      [&](const aiNode* node, const glm::mat4& parent) {
        if (!node || foundMesh) return;
        const glm::mat4 global = parent * AiToGlmMat4(node->mTransformation);
        if (node->mNumMeshes > 0) {
          meshGlobal = global;
          foundMesh = true;
          return;
        }
        for (unsigned i = 0; i < node->mNumChildren; ++i) {
          findMesh(node->mChildren[i], global);
        }
      };
  findMesh(scene_->mRootNode, glm::mat4(1.0f));

  std::function<void(const aiNode*, const glm::mat4&)> fixOffsets =
      [&](const aiNode* node, const glm::mat4& parent) {
        if (!node) return;
        const glm::mat4 global = parent * AiToGlmMat4(node->mTransformation);
        auto it = boneInfoMap.find(node->mName.data);
        if (it != boneInfoMap.end()) {
          it->second.offset = glm::inverse(global) * meshGlobal;
        }
        for (unsigned i = 0; i < node->mNumChildren; ++i) {
          fixOffsets(node->mChildren[i], global);
        }
      };
  fixOffsets(scene_->mRootNode, glm::mat4(1.0f));
}

bool Model::finishLoadFromScene(const std::string& directoryHint, bool isFbx) {
  if (!scene_ || !scene_->mRootNode || (scene_->mFlags & AI_SCENE_FLAGS_INCOMPLETE)) {
    return false;
  }

  directory = directoryHint;
  isFbxAsset_ = isFbx;
  // 与 AR CatalogEntry FBX 一致：和灯立方体（~0.05）同一世界尺度
  preferredWorldScale_ = isFbx ? 0.018f : 1.0f;
  fallbackTex_ = makeSolidTexture(235, 238, 245);

  // ①+② 解析顶点/骨权重，并在 Mesh 构造时上传 VAO（静态 mesh 带入节点全局矩阵）
  parseNode(scene_->mRootNode, scene_, glm::mat4(1.0f));

  // FBX：在挂 Animation 之前重算 offset（Animation 会拷贝 boneInfoMap）
  if (isFbx) {
    recalculateFbxBoneOffsets();
  }

  // ③ 动画挂载（与顶点分开；共享 boneInfoMap）
  loadAnimations(scene_);
  detectVehicleAsset(scene_);

  // 蒙皮车：用绑定姿势骨矩阵算包围盒（局部顶点会把轮子/车身拆散）
  if (!animations.empty()) {
    Animator bindPose(animations[0]);
    bindPose.setPlaying(false);
    bindPose.updateAnimation(0.0f);
    computeBounds(&bindPose.getFinalBoneMatrices());
  } else {
    computeBounds(nullptr);
  }

  // 「正面」：glTF 车（含 Benz 蒙皮）常背对 → π；人模/FBX → 0
  const bool characterSkinned = (boneCount > 0 && !isVehicleAsset_);
  preferredFrontYawOffset_ = (isFbx || characterSkinned) ? 0.0f : 3.14159265359f;
  return !meshes.empty();
}

bool Model::loadModel(const std::string& path) {
  destroy();

  importer_ = std::make_unique<Assimp::Importer>();
  scene_ = importer_->ReadFile(path, kAssimpFlags);
  if (!scene_) {
    std::cerr << "Assimp load failed: " << importer_->GetErrorString() << "\n";
    importer_.reset();
    return false;
  }

  const auto slash = path.find_last_of("/\\");
  const std::string dir = (slash == std::string::npos) ? "." : path.substr(0, slash);
  const bool isFbx = path.size() >= 4 &&
                     (path.compare(path.size() - 4, 4, ".fbx") == 0 ||
                      path.compare(path.size() - 4, 4, ".FBX") == 0);
  if (!finishLoadFromScene(dir, isFbx)) {
    destroy();
    return false;
  }
  return true;
}

bool Model::loadModelFromMemory(const void* data, size_t size, const char* hint) {
  destroy();
  if (!data || size == 0) return false;

  importer_ = std::make_unique<Assimp::Importer>();
  scene_ = importer_->ReadFileFromMemory(data, size, kAssimpFlags, hint ? hint : ".obj");
  if (!scene_) {
    std::cerr << "Assimp memory load failed: " << importer_->GetErrorString() << "\n";
    importer_.reset();
    return false;
  }

  const char* h = hint ? hint : ".obj";
  const bool isFbx = (std::strstr(h, "fbx") != nullptr || std::strstr(h, "FBX") != nullptr);
  if (!finishLoadFromScene(".", isFbx)) {
    destroy();
    return false;
  }
  return true;
}

void Model::Draw(const Shader& shader,
                 const std::map<int, glm::mat4>* boneMatrices) const {
  for (const auto& mesh : meshes) {
    mesh.Draw(shader, boneMatrices);
  }
}

void Model::destroy() {
  // 先丢 Animation（持有 scene_ 指针），再释放 Assimp / GL
  for (Animation* a : animations) delete a;
  animations.clear();
  boneInfoMap.clear();
  boneCount = 0;

  // 先清 CPU 顶点再删 VAO，大模型（Benz 数百 mesh）尽快还内存
  for (auto& mesh : meshes) {
    mesh.vertices.clear();
    mesh.vertices.shrink_to_fit();
    mesh.indices.clear();
    mesh.indices.shrink_to_fit();
    mesh.textures.clear();
    mesh.bonePalette.clear();
    mesh.destroy();
  }
  meshes.clear();

  for (auto& t : textures_loaded) {
    if (t.id) glDeleteTextures(1, &t.id);
    t.id = 0;
  }
  textures_loaded.clear();
  if (fallbackTex_) {
    glDeleteTextures(1, &fallbackTex_);
    fallbackTex_ = 0;
  }

  scene_ = nullptr;
  importer_.reset();  // 释放 Assimp 场景（含内嵌贴图解码缓冲）
  center_ = glm::vec3(0.0f);
  radius_ = 1.0f;
  preferredWorldScale_ = 1.0f;
  preferredFrontYawOffset_ = 0.0f;
  isFbxAsset_ = false;
  isVehicleAsset_ = false;
}
