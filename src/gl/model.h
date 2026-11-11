//
// model.h — same role as GL-ARKit/ARKit/GL/model.h
// Load Assimp scene into meshes; Draw() submits geometry (not a full-frame render).
//

#ifndef CORE_GL_MODEL_H_
#define CORE_GL_MODEL_H_

#include "mesh.h"
#include "shader.h"

#include <string>
#include <vector>

#include <glm/glm.hpp>

class Model {
 public:
  std::vector<Texture> textures_loaded;
  std::vector<Mesh> meshes;
  std::string directory;

  Model() = default;
  explicit Model(const std::string& path) { loadModel(path); }

  bool loadModel(const std::string& path);
  /** Load from memory (OHOS rawfile / asset bytes). hint e.g. ".obj". */
  bool loadModelFromMemory(const void* data, size_t size, const char* hint = ".obj");
  /** Submit all meshes with the current shader (AR Model::Draw). */
  void Draw(const Shader& shader) const;
  void destroy();

  const glm::vec3& center() const { return center_; }
  float radius() const { return radius_; }
  bool loaded() const { return !meshes.empty(); }

 private:
  glm::vec3 center_{0.0f};
  float radius_{1.0f};
  GLuint fallbackTex_ = 0;

  void processNode(const void* node, const void* scene);
  Mesh processMesh(const void* mesh, const void* scene);
  std::vector<Texture> loadMaterialTextures(const void* mat, int type, const std::string& typeName,
                                            const void* scene);
  unsigned int textureFromFile(const char* path, const std::string& directory);
  unsigned int makeSolidTexture(unsigned char r, unsigned char g, unsigned char b);
};

#endif /* CORE_GL_MODEL_H_ */
