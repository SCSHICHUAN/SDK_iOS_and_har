//
// created by stan 2026-11-18
// mesh.h — Mesh：生产 VAO
//
// 构造时 setupMesh：顶点/索引（含骨 ID、权重）上传 GPU → 生成 VAO/VBO/EBO。
// Draw：绑本片 VAO（+贴图）→ 可选上传骨矩阵 → glDrawElements。
// 骨 ID 只在 load 进 VBO 一次；每帧只换矩阵表（见 Animator），不重传顶点。
//

#ifndef CORE_GL_MESH_H_
#define CORE_GL_MESH_H_

#include "gl_platform.h"
#include "shader.h"

#include <cctype>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <glm/glm.hpp>

static constexpr int kMaxBoneInfluence = 4;
static constexpr int kMaxBonesUniform = 100;  // 与 lighting-vs MAX_BONES 一致

struct Vertex {
  glm::vec3 Position{0.0f};
  glm::vec3 Normal{0.0f, 1.0f, 0.0f};
  glm::vec2 TexCoords{0.0f};
  /** ES 用 float 传骨 id（layout 5）；-1 = 无影响 */
  float BoneIDs[kMaxBoneInfluence];
  float Weights[kMaxBoneInfluence];

  Vertex() {
    for (int i = 0; i < kMaxBoneInfluence; ++i) {
      BoneIDs[i] = -1.0f;
      Weights[i] = 0.0f;
    }
  }

  /** 写入本 mesh 局部骨槽（最多 4 根；满则替换权最小） */
  void addBoneData(int boneID, float weight) {
    for (int i = 0; i < kMaxBoneInfluence; ++i) {
      if (static_cast<int>(BoneIDs[i]) == boneID) {
        Weights[i] += weight;
        return;
      }
    }
    for (int i = 0; i < kMaxBoneInfluence; ++i) {
      if (BoneIDs[i] < 0.0f) {
        BoneIDs[i] = static_cast<float>(boneID);
        Weights[i] = weight;
        return;
      }
    }
    int minIdx = 0;
    float minW = Weights[0];
    for (int i = 1; i < kMaxBoneInfluence; ++i) {
      if (Weights[i] < minW) {
        minW = Weights[i];
        minIdx = i;
      }
    }
    if (weight > minW) {
      BoneIDs[minIdx] = static_cast<float>(boneID);
      Weights[minIdx] = weight;
    }
  }
};

struct Texture {
  unsigned int id = 0;
  std::string type;
  std::string path;
};

class Mesh {
 public:
  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;
  std::vector<Texture> textures;
  /** 本 mesh 局部骨槽 → 全局骨 id；空 = 无蒙皮 */
  std::vector<int> bonePalette;
  /** glTF baseColorFactor / 无贴图时的固有色（SU7 车漆等） */
  glm::vec4 materialColor{1.0f, 1.0f, 1.0f, 1.0f};
  /** whiteMan 等头发 mesh：无贴图时用 materialColor 固色，双面 */
  bool isHair = false;
  unsigned int VAO = 0;

  Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices,
       std::vector<Texture> textures, std::vector<int> bonePalette = {})
      : vertices(std::move(vertices)),
        indices(std::move(indices)),
        textures(std::move(textures)),
        bonePalette(std::move(bonePalette)) {
    setupMesh();  // 此处产出 VAO/VBO/EBO（须已 MakeCurrent）
  }

  Mesh(const Mesh&) = delete;
  Mesh& operator=(const Mesh&) = delete;

  Mesh(Mesh&& other) noexcept
      : vertices(std::move(other.vertices)),
        indices(std::move(other.indices)),
        textures(std::move(other.textures)),
        bonePalette(std::move(other.bonePalette)),
        materialColor(other.materialColor),
        isHair(other.isHair),
        VAO(other.VAO),
        VBO(other.VBO),
        EBO(other.EBO) {
    other.VAO = other.VBO = other.EBO = 0;
  }

  Mesh& operator=(Mesh&& other) noexcept {
    if (this != &other) {
      destroy();
      vertices = std::move(other.vertices);
      indices = std::move(other.indices);
      textures = std::move(other.textures);
      bonePalette = std::move(other.bonePalette);
      materialColor = other.materialColor;
      isHair = other.isHair;
      VAO = other.VAO;
      VBO = other.VBO;
      EBO = other.EBO;
      other.VAO = other.VBO = other.EBO = 0;
    }
    return *this;
  }

  ~Mesh() { destroy(); }

  /**
   * 提交本片几何。
   * @param globalBoneMatrices  Animator 产出的 globalBoneId→skinMatrix；
   *                            nullptr / 空 palette → 上传单位矩阵（静态网格）
   */
  void Draw(const Shader& shader,
            const std::map<int, glm::mat4>* globalBoneMatrices = nullptr) const {
    // 只上传本 mesh 用到的骨槽（一次 setMat4Array）；勿对每 mesh 循环 100 次
    // glGetUniformLocation+字符串，Benz 近 500 mesh 会直接把手机打烫。
    const int n = std::min(static_cast<int>(bonePalette.size()), kMaxBonesUniform);
    if (n > 0) {
      glm::mat4 palette[kMaxBonesUniform];
      const glm::mat4 identity(1.0f);
      for (int local = 0; local < n; ++local) {
        palette[local] = identity;
        if (globalBoneMatrices) {
          auto it = globalBoneMatrices->find(bonePalette[local]);
          if (it != globalBoneMatrices->end()) palette[local] = it->second;
        }
      }
      shader.setMat4Array("finalBonesMatrices", palette, n);
    }

    unsigned int specularNr = 0;
    bool hasDiffuse = false;
    bool useAlphaCutout = false;

    static GLuint whiteTex = 0;
    if (!whiteTex) {
      unsigned char white[] = {255, 255, 255, 255};
      glGenTextures(1, &whiteTex);
      glBindTexture(GL_TEXTURE_2D, whiteTex);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    for (unsigned int i = 0; i < textures.size(); ++i) {
      glActiveTexture(GL_TEXTURE0 + i);
      glBindTexture(GL_TEXTURE_2D, textures[i].id);
      std::string name = textures[i].type;
      if (name == "texture_diffuse") {
        name = "material.diffuse";
        hasDiffuse = true;
        // Wolf_Fur.jpg：黑毛+白底，须开 cutout，否则毛发整片发黑
        std::string pl = textures[i].path;
        for (char& c : pl) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
        if (pl.find("fur") != std::string::npos || pl.find("hair") != std::string::npos) {
          useAlphaCutout = true;
        }
      } else if (name == "texture_specular") {
        name = "material.specular";
        ++specularNr;
      }
      shader.setInt(name, static_cast<int>(i));
    }

    if (!hasDiffuse) {
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, whiteTex);
      shader.setInt("material.diffuse", 0);
      shader.setInt("material.specular", 0);
    } else if (specularNr == 0) {
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, textures.empty() ? whiteTex : textures[0].id);
      shader.setInt("material.specular", 1);
    }

    // 头发无贴图：shader 直接用 materialColor（金发）；车漆无贴图：白图×factor
    if (isHair) useAlphaCutout = true;
    shader.setBool("useAlphaCutout", useAlphaCutout);
    shader.setBool("hairSolidColor", isHair && !hasDiffuse);
    shader.setVec4("materialColor", materialColor);

    // Fur/头发双面：关剔除，避免卡片式毛发背面黑洞
    const GLboolean wasCull = glIsEnabled(GL_CULL_FACE);
    if (useAlphaCutout || isHair) glDisable(GL_CULL_FACE);

    // 玻璃等 BLEND 材质
    const bool needBlend = materialColor.a < 0.999f;
    GLboolean wasBlend = GL_FALSE;
    if (needBlend) {
      wasBlend = glIsEnabled(GL_BLEND);
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT,
                   nullptr);
    glBindVertexArray(0);

    if (needBlend && !wasBlend) glDisable(GL_BLEND);
    if (useAlphaCutout && wasCull) glEnable(GL_CULL_FACE);
    glActiveTexture(GL_TEXTURE0);
  }

  void destroy() {
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (EBO) glDeleteBuffers(1, &EBO);
    VAO = VBO = EBO = 0;
  }

 private:
  unsigned int VBO = 0;
  unsigned int EBO = 0;

  /**
   * Load 阶段：上传 VBO/EBO，配置 VAO layout。
   * 配完解绑 = 清「当前绑定」，不是丢掉 VAO；Draw 时再绑同一份。
   */
  void setupMesh() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
                 vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
                 indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, Position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, Normal)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, TexCoords)));
    // layout 5/6：骨 ID / 权重（进 VBO，只传一次）
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, BoneIDs)));
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, Weights)));

    glBindVertexArray(0);  // 解绑当前状态；对象仍在
  }
};

#endif /* CORE_GL_MESH_H_ */
