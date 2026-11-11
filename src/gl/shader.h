//
// shader.h — same role as GL-ARKit/ARKit/GL/shader.h (file + FromSource)
//

#ifndef CORE_GL_SHADER_H_
#define CORE_GL_SHADER_H_

#include "gl_platform.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#if defined(__ANDROID__)
#include <android/log.h>
#define CORE_GL_LOG(...) __android_log_print(ANDROID_LOG_ERROR, "core_gl", __VA_ARGS__)
#else
#define CORE_GL_LOG(...) \
  do {                   \
    std::cerr << __VA_ARGS__ << std::endl; \
  } while (0)
#endif

class Shader {
 public:
  unsigned int ID = 0;

  Shader() = default;

  Shader(const Shader&) = delete;
  Shader& operator=(const Shader&) = delete;

  Shader(Shader&& other) noexcept : ID(other.ID) { other.ID = 0; }
  Shader& operator=(Shader&& other) noexcept {
    if (this != &other) {
      destroy();
      ID = other.ID;
      other.ID = 0;
    }
    return *this;
  }

  ~Shader() { destroy(); }

  /** Load GLSL from filesystem paths (ARKit style). */
  Shader(const char* vertexPath, const char* fragmentPath) {
    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
      vShaderFile.open(vertexPath);
      fShaderFile.open(fragmentPath);
      std::stringstream vShaderStream, fShaderStream;
      vShaderStream << vShaderFile.rdbuf();
      fShaderStream << fShaderFile.rdbuf();
      vShaderFile.close();
      fShaderFile.close();
      vertexCode = vShaderStream.str();
      fragmentCode = fShaderStream.str();
    } catch (std::ifstream::failure& e) {
      std::cerr << "ERROR: load shader file: " << e.what() << std::endl;
      return;
    }
    compile(vertexCode.c_str(), fragmentCode.c_str());
  }

  /** Compile from in-memory source (SDK default viewer). */
  static Shader FromSource(const char* vertexSrc, const char* fragmentSrc) {
    Shader s;
    s.compile(vertexSrc, fragmentSrc);
    return s;
  }

  void use() const { glUseProgram(ID); }

  void setBool(const std::string& name, bool value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), static_cast<int>(value));
  }
  void setInt(const std::string& name, int value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
  }
  void setFloat(const std::string& name, float value) const {
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
  }
  void setMat4(const std::string& name, const glm::mat4& mat) const {
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
  }
  void setMat4Array(const std::string& name, const glm::mat4* mats, int count) const {
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), count, GL_FALSE,
                       glm::value_ptr(mats[0]));
  }
  void setVec3(const std::string& name, float x, float y, float z) const {
    glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
  }
  void setVec3(const std::string& name, const glm::vec3& value) const {
    glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(value));
  }
  void setVec4(const std::string& name, const glm::vec4& value) const {
    glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(value));
  }

  void destroy() {
    if (ID) {
      glDeleteProgram(ID);
      ID = 0;
    }
  }

 private:
  void compile(const char* vShaderCode, const char* fShaderCode) {
    unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, nullptr);
    glCompileShader(vertex);
    int vOk = 1;
    checkCompileErrors(vertex, "VERTEX", nullptr);
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &vOk);

    unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, nullptr);
    glCompileShader(fragment);
    int fOk = 1;
    checkCompileErrors(fragment, "FRAGMENT", nullptr);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &fOk);

    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    if (!vOk || !fOk) {
#if defined(__ANDROID__)
      CORE_GL_LOG("skip link: compile failed v=%d f=%d", vOk, fOk);
#else
      CORE_GL_LOG("skip link: compile failed");
#endif
      glDeleteProgram(ID);
      ID = 0;
    } else {
      glLinkProgram(ID);
      checkCompileErrors(ID, "PROGRAM", &ID);
    }
    glDeleteShader(vertex);
    glDeleteShader(fragment);
  }

  static void checkCompileErrors(unsigned int object, const std::string& type, unsigned int* idOut) {
    int success = 0;
    char infoLogs[1024];
    if (type != "PROGRAM") {
      glGetShaderiv(object, GL_COMPILE_STATUS, &success);
      if (!success) {
        glGetShaderInfoLog(object, 1024, nullptr, infoLogs);
#if defined(__ANDROID__)
        CORE_GL_LOG("shader compile %s: %s", type.c_str(), infoLogs);
#else
        CORE_GL_LOG(std::string("ERROR: shader compile type:") + type + "\n" + infoLogs);
#endif
        if (idOut) *idOut = 0;
      }
    } else {
      glGetProgramiv(object, GL_LINK_STATUS, &success);
      if (!success) {
        glGetProgramInfoLog(object, 1024, nullptr, infoLogs);
#if defined(__ANDROID__)
        CORE_GL_LOG("shader link: %s", infoLogs);
#else
        CORE_GL_LOG(std::string("ERROR: shader link type:") + type + "\n" + infoLogs);
#endif
        glDeleteProgram(object);
        if (idOut) *idOut = 0;
      }
    }
  }
};

#endif /* CORE_GL_SHADER_H_ */
