//
// mesh.h — same role as GL-ARKit/ARKit/GL/mesh.h (no skin/morph yet).
// Draw() = bind VAO/textures + glDraw*; full-frame render is RendererData.
//

#ifndef CORE_GL_MESH_H_
#define CORE_GL_MESH_H_

#include "gl_platform.h"
#include "shader.h"

#include <string>
#include <utility>
#include <vector>

#include <glm/glm.hpp>

struct Vertex {
  glm::vec3 Position;
  glm::vec3 Normal;
  glm::vec2 TexCoords;
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
  unsigned int VAO = 0;

  Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices,
       std::vector<Texture> textures)
      : vertices(std::move(vertices)),
        indices(std::move(indices)),
        textures(std::move(textures)) {
    setupMesh();
  }

  Mesh(const Mesh&) = delete;
  Mesh& operator=(const Mesh&) = delete;

  Mesh(Mesh&& other) noexcept
      : vertices(std::move(other.vertices)),
        indices(std::move(other.indices)),
        textures(std::move(other.textures)),
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
      VAO = other.VAO;
      VBO = other.VBO;
      EBO = other.EBO;
      other.VAO = other.VBO = other.EBO = 0;
    }
    return *this;
  }

  ~Mesh() { destroy(); }

  void Draw(const Shader& shader) const {
    // Same material.* binding as GL-ARKit mesh.h (lighting / former colors).
    unsigned int specularNr = 0;
    bool hasDiffuse = false;

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
      // ES: unbound specular sampler → undefined; reuse diffuse or white.
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, textures.empty() ? whiteTex : textures[0].id);
      shader.setInt("material.specular", 1);
    }

    shader.setBool("useAlphaCutout", false);
    shader.setBool("hairSolidColor", false);
    shader.setVec4("materialColor", glm::vec4(1.0f));

    // lighting-vs aBoneIDs(5)/aWeights(6)：无骨骼网格用常量，避免默认 (0,0,0,1) 误蒙皮
    glVertexAttrib4f(5, -1.0f, -1.0f, -1.0f, -1.0f);
    glVertexAttrib4f(6, 0.0f, 0.0f, 0.0f, 0.0f);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT,
                   nullptr);
    glBindVertexArray(0);
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
    glBindVertexArray(0);
  }
};

#endif /* CORE_GL_MESH_H_ */
