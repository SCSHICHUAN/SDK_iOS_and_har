//
// renderer_data.h — RendererData：场景静态数据袋
//
// 只放字段：Model / Shader / Camera / 灯 / 浏览姿态等。
// 模型数据由 Model 加载；手势改字段见 viewport_input；整帧绘制见 render。
// 本文件只负责 init（shader/灯缓冲）与 destroy。
//

#ifndef CORE_GL_RENDERER_DATA_H_
#define CORE_GL_RENDERER_DATA_H_

#include "arcball_mobile.h"
#include "camera.h"
#include "debut_sequence.h"
#include "inertia.h"
#include "model.h"
#include "shader.h"

#include <glm/glm.hpp>

class RendererData {
 public:
  // —— members（静态场景状态）——
  Model model;              // 已加载网格/动画资源
  Shader shader;             // 模型 lighting program
  Shader lampShader;         // 灯立方体 program
  Camera camera;             // 观察相机
  ArcballMobile arcball;     // 模型旋转姿态
  Inertia inertia;           // 拖动/滑行位移队列
  DebutSequence debut;       // 加载后入场（推进 + 自转）

  unsigned int lightCubeVAO = 0;  // 灯标记几何
  unsigned int lightCubeVBO = 0;

  int width = 1;             // 视口宽
  int height = 1;            // 视口高
  float camYaw = 30.0f;      // 相机轨道偏航
  float camPitch = 20.0f;    // 相机轨道俯仰
  float distance = 3.0f;     // 相机到模型中心距离（模型已按 modelScale 归一化）
  float camPan = 0.0f;       // 相机左右平移（沿水平 Right，改观察位置）
  float modelScale = 1.0f;   // 显示缩放：1/radius，FBX 大单位（如 Wolf）才能进画面
  float lightX = -0.4f;      // 点光源位置（世界坐标）
  float lightY = 1.15f;
  float lightZ = 0.0f;
  bool pointLightEnabled = true;          // 「灯」点光源开关
  glm::vec3 pointLightColor{1.0f, 1.0f, 1.0f};  // 色卡：红/黄/天蓝/白
  bool dirLightEnabled = true;   // 太阳（平行光）
  bool spotLightEnabled = true;  // 手电（聚光，跟相机）
  float elapsed = 0.0f;      // 累计时间
  bool ready = false;        // init 是否成功

  // —— methods（仅资源生命周期）——
  RendererData() = default;
  ~RendererData() { destroy(); }

  RendererData(const RendererData&) = delete;
  RendererData& operator=(const RendererData&) = delete;

  /**
   * 功能：编译 shader、创建灯立方体缓冲，启用深度测试。
   * 参数：无
   * 输入：须 GLES Context 已 current
   * 返回：true 成功；false shader 失败
   */
  bool init();

  /**
   * 功能：释放 model/shader/灯 VAO，ready=false。
   * 参数/输入：无
   * 返回：无
   */
  void destroy();

  /**
   * 功能：是否已有可画网格。
   * 参数/输入：无
   * 返回：model.loaded()
   */
  bool hasModel() const { return model.loaded(); }
};

#endif /* CORE_GL_RENDERER_DATA_H_ */
