//
// created by stan 2026-11-18
// viewport_input.h — ViewportInput：手势改 RendererData
//
// 不加载、不画、不播动画；只根据触摸/缩放改 distance、inertia、arcball、灯位置等字段。
//

#ifndef CORE_GL_VIEWPORT_INPUT_H_
#define CORE_GL_VIEWPORT_INPUT_H_

#include "renderer_data.h"

namespace ViewportInput {

/**
 * 功能：Model 加载成功后重设观察距离并清浏览状态。
 * 参数：data — 场景数据
 * 输入：data.model 已 loaded
 * 返回：无
 */
void onModelLoaded(RendererData& data);

/**
 * 功能：更新视口宽高、arcball、glViewport。
 * 参数：data；width/height — 像素
 * 输入：data, width, height
 * 返回：无
 */
void resize(RendererData& data, int width, int height);

/**
 * 功能：触摸拖动（MOVE）→ inertia 入队。
 * 参数：data；dx/dy — 像素增量（右+、屏下+）
 * 输入：data, dx, dy
 * 返回：无
 */
void orbit(RendererData& data, float dx, float dy);

/**
 * 功能：触摸抬起 → inertia 松手滑行 + arcball.endDrag。
 * 参数：data
 * 输入：data
 * 返回：无
 */
void orbitEnd(RendererData& data);

/**
 * 功能：设置拖动灵敏度与松手刹车。
 * 参数：data；sensitivity — 齿轮比；brakeFactor — 仅松手刹车
 * 输入：data, sensitivity, brakeFactor
 * 返回：无
 */
void setOrbitFeel(RendererData& data, float sensitivity, float brakeFactor);

/**
 * 功能：捏合缩放，改 distance。
 * 参数：data；scale — >1 拉近；(0,1) 推远
 * 输入：data, scale
 * 返回：无
 */
void pinch(RendererData& data, float scale);

/**
 * 功能：沿视线推拉，改 distance。
 * 参数：data；amount — >0 靠近
 * 输入：data, amount
 * 返回：无
 */
void dolly(RendererData& data, float amount);

/**
 * 功能：点光源沿世界 X 平移。
 * 参数：data；amount — >0 向右
 * 输入：data, amount
 * 返回：无
 */
void lightMoveX(RendererData& data, float amount);

/**
 * 功能：点光源沿世界 Z 平移（前进/后退按钮控制灯时用）。
 * 参数：data；amount — >0 沿 +Z（前方习惯与按钮「前进」一致）
 * 输入：data, amount
 * 返回：无
 */
void lightMoveZ(RendererData& data, float amount);

/**
 * 功能：屏幕 XY 拖灯光（范围随模型尺度放大）。
 * 参数：data；dx/dy — 与 orbit 同坐标系
 * 输入：data, dx, dy
 * 返回：无
 */
void lightDrag(RendererData& data, float dx, float dy);

/**
 * 功能：开关平行光（太阳）。
 */
void setDirLightEnabled(RendererData& data, bool enabled);

/**
 * 功能：开关聚光灯（手电，跟相机）。
 */
void setSpotLightEnabled(RendererData& data, bool enabled);

/**
 * 功能：开关点光源（灯）。
 */
void setPointLightEnabled(RendererData& data, bool enabled);

/**
 * 功能：设置点光源颜色（色卡 RGB，0~1）。
 */
void setPointLightColor(RendererData& data, float r, float g, float b);

/**
 * 功能：相机左右平移位置（左右按钮，沿水平 Right）。
 * 参数：amount — >0 向右；<0 向左
 */
void cameraPan(RendererData& data, float amount);

}  // namespace ViewportInput

#endif /* CORE_GL_VIEWPORT_INPUT_H_ */
