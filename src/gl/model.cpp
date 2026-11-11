#include "model.h"

#include <iostream>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace {

unsigned int loadTexture2D(const std::string& path) {
  int w = 0, h = 0, comp = 0;
  stbi_set_flip_vertically_on_load(true);
  unsigned char* data = stbi_load(path.c_str(), &w, &h, &comp, 4);
  if (!data) {
    std::cerr << "stb_image failed: " << path << " (" << stbi_failure_reason() << ")\n";
    return 0;
  }
  unsigned int tex = 0;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glBindTexture(GL_TEXTURE_2D, 0);
  stbi_image_free(data);
  return tex;
}

}  // namespace

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

unsigned int Model::textureFromFile(const char* path, const std::string& directory) {
  const std::string full = directory + "/" + path;
  for (const auto& t : textures_loaded) {
    if (t.path == full) return t.id;
  }
  unsigned int id = loadTexture2D(full);
  if (id) {
    Texture tex;
    tex.id = id;
    tex.type = "texture_diffuse";
    tex.path = full;
    textures_loaded.push_back(tex);
  }
  return id;
}

std::vector<Texture> Model::loadMaterialTextures(const void* matPtr, int type,
                                                 const std::string& typeName,
                                                 const void* /*scene*/) {
  const aiMaterial* mat = static_cast<const aiMaterial*>(matPtr);
  const aiTextureType aiType = static_cast<aiTextureType>(type);
  std::vector<Texture> textures;
  const unsigned count = mat->GetTextureCount(aiType);
  for (unsigned i = 0; i < count; ++i) {
    aiString str;
    mat->GetTexture(aiType, i, &str);
    unsigned int id = textureFromFile(str.C_Str(), directory);
    if (!id) continue;
    Texture tex;
    tex.id = id;
    tex.type = typeName;
    tex.path = directory + "/" + str.C_Str();
    textures.push_back(tex);
  }
  return textures;
}

Mesh Model::processMesh(const void* meshPtr, const void* scenePtr) {
  const aiMesh* mesh = static_cast<const aiMesh*>(meshPtr);
  const aiScene* scene = static_cast<const aiScene*>(scenePtr);

  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;
  std::vector<Texture> textures;
  vertices.reserve(mesh->mNumVertices);

  for (unsigned i = 0; i < mesh->mNumVertices; ++i) {
    Vertex v;
    v.Position = {mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z};
    if (mesh->HasNormals()) {
      v.Normal = {mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z};
    } else {
      v.Normal = {0, 1, 0};
    }
    if (mesh->mTextureCoords[0]) {
      v.TexCoords = {mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y};
    } else {
      v.TexCoords = {0, 0};
    }
    vertices.push_back(v);
  }

  for (unsigned f = 0; f < mesh->mNumFaces; ++f) {
    const aiFace& face = mesh->mFaces[f];
    for (unsigned j = 0; j < face.mNumIndices; ++j) {
      indices.push_back(face.mIndices[j]);
    }
  }

  if (mesh->mMaterialIndex < scene->mNumMaterials) {
    aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
    auto diffuse = loadMaterialTextures(mat, aiTextureType_DIFFUSE, "texture_diffuse", scene);
    textures.insert(textures.end(), diffuse.begin(), diffuse.end());
  }
  if (textures.empty() && fallbackTex_) {
    Texture t;
    t.id = fallbackTex_;
    t.type = "texture_diffuse";
    t.path = "";
    textures.push_back(t);
  }

  return Mesh(std::move(vertices), std::move(indices), std::move(textures));
}

void Model::processNode(const void* nodePtr, const void* scenePtr) {
  const aiNode* node = static_cast<const aiNode*>(nodePtr);
  const aiScene* scene = static_cast<const aiScene*>(scenePtr);
  for (unsigned i = 0; i < node->mNumMeshes; ++i) {
    aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
    meshes.push_back(processMesh(mesh, scene));
  }
  for (unsigned i = 0; i < node->mNumChildren; ++i) {
    processNode(node->mChildren[i], scene);
  }
}

namespace {

constexpr unsigned kAssimpFlags =
    aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs |
    aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace;

}  // namespace

bool Model::loadModel(const std::string& path) {
  destroy();

  Assimp::Importer importer;
  const aiScene* scene = importer.ReadFile(path, kAssimpFlags);
  if (!scene || !scene->mRootNode || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)) {
    std::cerr << "Assimp load failed: " << importer.GetErrorString() << "\n";
    return false;
  }

  const auto slash = path.find_last_of("/\\");
  directory = (slash == std::string::npos) ? "." : path.substr(0, slash);
  // Untextured meshes (cube.obj): near-white albedo; viewer lowers ambient slightly for shade.
  fallbackTex_ = makeSolidTexture(235, 238, 245);

  processNode(scene->mRootNode, scene);

  glm::vec3 bmin(1e9f), bmax(-1e9f);
  for (const auto& mesh : meshes) {
    for (const auto& v : mesh.vertices) {
      bmin = glm::min(bmin, v.Position);
      bmax = glm::max(bmax, v.Position);
    }
  }
  center_ = 0.5f * (bmin + bmax);
  radius_ = glm::length(bmax - bmin) * 0.5f;
  if (radius_ < 1e-4f) radius_ = 1.0f;
  return !meshes.empty();
}

bool Model::loadModelFromMemory(const void* data, size_t size, const char* hint) {
  destroy();
  if (!data || size == 0) return false;

  Assimp::Importer importer;
  const aiScene* scene =
      importer.ReadFileFromMemory(data, size, kAssimpFlags, hint ? hint : ".obj");
  if (!scene || !scene->mRootNode || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)) {
    std::cerr << "Assimp memory load failed: " << importer.GetErrorString() << "\n";
    return false;
  }

  directory = ".";
  fallbackTex_ = makeSolidTexture(235, 238, 245);
  processNode(scene->mRootNode, scene);

  glm::vec3 bmin(1e9f), bmax(-1e9f);
  for (const auto& mesh : meshes) {
    for (const auto& v : mesh.vertices) {
      bmin = glm::min(bmin, v.Position);
      bmax = glm::max(bmax, v.Position);
    }
  }
  center_ = 0.5f * (bmin + bmax);
  radius_ = glm::length(bmax - bmin) * 0.5f;
  if (radius_ < 1e-4f) radius_ = 1.0f;
  return !meshes.empty();
}

void Model::Draw(const Shader& shader) const {
  for (const auto& mesh : meshes) {
    mesh.Draw(shader);
  }
}

void Model::destroy() {
  for (auto& mesh : meshes) {
    mesh.destroy();
  }
  meshes.clear();
  // Textures may be shared via textures_loaded; delete unique ids once.
  for (auto& t : textures_loaded) {
    if (t.id) glDeleteTextures(1, &t.id);
    t.id = 0;
  }
  textures_loaded.clear();
  if (fallbackTex_) {
    glDeleteTextures(1, &fallbackTex_);
    fallbackTex_ = 0;
  }
  center_ = glm::vec3(0.0f);
  radius_ = 1.0f;
}
