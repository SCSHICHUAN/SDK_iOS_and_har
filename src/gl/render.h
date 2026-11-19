//
// render.h — Render：整帧画到当前 FBO
// created by stan 2026-11-18
//
// 用 RendererData 里的模型/相机/灯：清屏 → 设 MVP/光照 → Model::Draw。
// 不加载模型、不播动画；骨矩阵由调用方（Animator）传入。
//

#ifndef CORE_GL_RENDER_H_
#define CORE_GL_RENDER_H_

#include "renderer_data.h"

#include <map>

#include <glm/glm.hpp>

class Render {
 public:
  /**
   * 功能：向当前 FBO 画一帧。
   * 参数：
   *   data          — 场景静态数据（Model/Shader/Camera/灯/浏览姿态）
   *   boneMatrices  — Animator 产出的骨矩阵；nullptr = 静态网格
   * 输入：data 已 init 且（通常）已 load；Context/FBO 已由平台绑好
   * 返回：无
   */
  static void frame(RendererData& data,
                    const std::map<int, glm::mat4>* boneMatrices = nullptr);
};

#endif /* CORE_GL_RENDER_H_ */
