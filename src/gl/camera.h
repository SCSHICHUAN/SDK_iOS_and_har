//
// camera.h — from GL-ARKit/ARKit/GL/Camera.h (normalized for core SDK)
//

#ifndef CORE_GL_CAMERA_H_
#define CORE_GL_CAMERA_H_

#include "gl_platform.h"

#include <cmath>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum Camera_Movement {
  FORWARD,
  BACKWARD,
  LEFT,
  RIGHT,
  UPWARD,
  DOWN
};

const float kCameraYaw = -90.f;
const float kCameraPitch = 0.0f;
const float kCameraSpeed = 1.8f;
const float kCameraStrafeSpeed = 0.45f;
const float kCameraSensitivity = 0.1f;
const float kCameraZoom = 45.0f;

class Camera {
 public:
  glm::vec3 Position;
  glm::vec3 Front;
  glm::vec3 Up;
  glm::vec3 Right;
  glm::vec3 WorldUp;
  float Yaw;
  float Pitch;
  float MovementSpeed;
  float MouseSensitivity;
  float Zoom;

  Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
         glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
         float yaw = kCameraYaw,
         float pitch = kCameraPitch)
      : Front(glm::vec3(0.0f, 0.0f, -1.0f)),
        MovementSpeed(kCameraSpeed),
        MouseSensitivity(kCameraSensitivity),
        Zoom(kCameraZoom) {
    Position = position;
    WorldUp = up;
    Yaw = yaw;
    Pitch = pitch;
    updateCameraVectors();
  }

  Camera(float posX, float posY, float posZ, float upX, float upY, float upZ,
         float yaw, float pitch)
      : Front(glm::vec3(0.0f, 0.0f, -1.0f)),
        MovementSpeed(kCameraSpeed),
        MouseSensitivity(kCameraSensitivity),
        Zoom(kCameraZoom) {
    Position = glm::vec3(posX, posY, posZ);
    WorldUp = glm::vec3(upX, upY, upZ);
    Yaw = yaw;
    Pitch = pitch;
    updateCameraVectors();
  }

  glm::mat4 GetViewMatrix() const {
    return glm::lookAt(Position, Position + Front, Up);
  }

  void ProcessKeyboard(Camera_Movement direction, float deltaTime) {
    float fwdVel = MovementSpeed * deltaTime;
    float sideVel = kCameraStrafeSpeed * deltaTime;
    glm::vec3 flatFront = Front;
    flatFront.y = 0.0f;
    if (glm::dot(flatFront, flatFront) > 1e-6f)
      flatFront = glm::normalize(flatFront);
    else
      flatFront = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 flatRight = glm::normalize(glm::cross(flatFront, WorldUp));

    if (direction == FORWARD) Position += Front * fwdVel;
    if (direction == BACKWARD) Position -= Front * fwdVel;
    if (direction == LEFT) Position -= flatRight * sideVel;
    if (direction == RIGHT) Position += flatRight * sideVel;
    if (direction == UPWARD) Position += Up * sideVel;
    if (direction == DOWN) Position -= Up * sideVel;
  }

  void LookAtPoint(glm::vec3 target) {
    glm::vec3 dir = target - Position;
    float len = glm::length(dir);
    if (len < 1e-5f) return;
    dir /= len;
    float py = dir.y;
    if (py > 0.99f) py = 0.99f;
    if (py < -0.99f) py = -0.99f;
    Pitch = glm::degrees(asinf(py));
    Yaw = glm::degrees(atan2f(dir.z, dir.x));
    updateCameraVectors();
  }

  void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true) {
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;
    Yaw += xoffset;
    Pitch += yoffset;
    if (constrainPitch) {
      if (Pitch > 89.0f) Pitch = 89.0f;
      if (Pitch < -89.0f) Pitch = -89.0f;
    }
    updateCameraVectors();
  }

  void ProcessMouseScroll(float yoffset) {
    Zoom -= yoffset;
    if (Zoom < 1.0f) Zoom = 1.0f;
    if (Zoom > 45.0f) Zoom = 45.0f;
  }

  /** Place camera on a sphere around `center` (model browse / orbit). */
  void SetOrbit(const glm::vec3& center, float distance, float yawDeg, float pitchDeg) {
    if (pitchDeg > 89.0f) pitchDeg = 89.0f;
    if (pitchDeg < -89.0f) pitchDeg = -89.0f;
    const float yawR = glm::radians(yawDeg);
    const float pitchR = glm::radians(pitchDeg);
    Position = glm::vec3(
        center.x + distance * std::cos(pitchR) * std::sin(yawR),
        center.y + distance * std::sin(pitchR),
        center.z + distance * std::cos(pitchR) * std::cos(yawR));
    LookAtPoint(center);
  }

 private:
  void updateCameraVectors() {
    glm::vec3 front;
    front.y = sin(glm::radians(Pitch));
    front.x = cos(glm::radians(Pitch)) * cos(glm::radians(Yaw));
    front.z = cos(glm::radians(Pitch)) * sin(glm::radians(Yaw));
    Front = glm::normalize(front);
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up = glm::normalize(glm::cross(Right, Front));
  }
};

#endif /* CORE_GL_CAMERA_H_ */
