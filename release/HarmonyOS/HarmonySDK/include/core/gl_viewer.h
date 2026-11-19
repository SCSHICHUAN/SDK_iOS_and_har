//
// gl_viewer.h — GLES 模型浏览器 C ABI（Swift / JNI / NAPI）
//
// 分层：本头文件 = 跨语言边界；
//       RendererData = 场景静态数据；Render = 整帧提交；Animator = 播放；
//       Pro = Surface/FBO。
// 约定：create/load/render 前须 makeCurrent GLES3；viewer==NULL 安全空操作。
//

#ifndef CORE_GL_VIEWER_H_
#define CORE_GL_VIEWER_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 不透明句柄。
 * 内部持有 RendererData + Animator；画帧走 Render::frame。
 */
typedef struct CoreGlViewer CoreGlViewer;

/**
 * 功能：创建模型浏览器实例（编译 shader、开深度测试、初始化刹车）。
 * 参数：无
 * 输入：无（但调用前须已 makeCurrent GLES3 上下文）
 * 返回：成功 → CoreGlViewer*；失败 → NULL
 */
CoreGlViewer* core_gl_viewer_create(void);

/**
 * 功能：销毁浏览器，释放模型 / shader / VAO 后 delete 句柄。
 * 参数：
 *   viewer — 实例句柄；可为 NULL（空操作）
 * 输入：viewer
 * 返回：无
 */
void core_gl_viewer_destroy(CoreGlViewer* viewer);

/**
 * 功能：从文件路径加载模型（Assimp：obj / fbx / glb / ...）。
 *       成功后按包围球重设观察距离，并 reset arcball / inertia。
 * 参数：
 *   viewer — 实例句柄，不可为 NULL
 *   path   — UTF-8 模型文件路径，不可为 NULL
 * 输入：viewer, path；当前 GLES 上下文须有效
 * 返回：0 成功；-1 参数非法或 Assimp 加载失败
 */
int core_gl_viewer_load(CoreGlViewer* viewer, const char* path);

/**
 * 功能：从内存字节加载模型（适合包内资源一次读入）。
 * 参数：
 *   viewer — 实例句柄，不可为 NULL
 *   data   — 模型二进制首地址，不可为 NULL
 *   size   — 字节长度，须 > 0
 *   hint   — Assimp 扩展名提示，如 ".obj" / ".glb"；传 NULL 时按 ".obj"
 * 输入：viewer, data[0..size), hint
 * 返回：0 成功；-1 参数非法或加载失败
 */
int core_gl_viewer_load_memory(CoreGlViewer* viewer, const void* data, int size,
                               const char* hint);

/**
 * 功能：视口尺寸变化时更新 glViewport，并同步 ArcballMobile 宽高（θ 用 Height）。
 * 参数：
 *   viewer — 实例句柄；NULL 则空操作
 *   width  — 视口宽（像素）；内部 clamp 到 ≥1
 *   height — 视口高（像素）；内部 clamp 到 ≥1
 * 输入：viewer, width, height
 * 返回：无
 */
void core_gl_viewer_resize(CoreGlViewer* viewer, int width, int height);

/**
 * 功能：向当前绑定的 framebuffer 画一帧（Pro FBO 或默认 FB）。
 *       先 Animator 插值骨矩阵（若有动画），再 Render::frame(RendererData, …)。
 * 参数：
 *   viewer — 实例句柄；NULL 则空操作
 * 输入：viewer；当前 FBO / GLES 上下文须已就绪
 * 返回：无
 */
void core_gl_viewer_render(CoreGlViewer* viewer);

/**
 * 功能：触摸拖动（MOVE）。屏幕位移 100% 入队 inertia；真正旋转在 render 里完成。
 *       转轴：dx → 物体局部 Y；dy → 相机 OX；
 *       幅度：θ = 2π · px / Height · sensitivity。
 * 参数：
 *   viewer — 实例句柄；NULL 则空操作
 *   dx     — 相对上一触摸事件的水平像素增量（右为正）
 *   dy     — 相对上一触摸事件的垂直像素增量（屏幕向下为正，与 Pro 一致）
 * 输入：viewer, dx, dy
 * 返回：无
 */
void core_gl_viewer_orbit(CoreGlViewer* viewer, float dx, float dy);

/**
 * 功能：触摸抬起（UP）。近窗估速启动松手滑行；并 arcball.endDrag。
 *       须与 orbit 成对调用；速度低于阈值则不滑。
 * 参数：
 *   viewer — 实例句柄；NULL 则空操作
 * 输入：viewer
 * 返回：无
 */
void core_gl_viewer_orbit_end(CoreGlViewer* viewer);

/**
 * 功能：设置浏览手感。两参数互不依赖（对标 Sketchfab / su7）。
 * 参数：
 *   viewer      — 实例句柄；NULL 则空操作
 *   sensitivity — 拖动齿轮比（OrbitControls rotateSpeed）。
 *                 默认 1：一屏高 ≈ 转一圈；越大拖动转得越多。非法值（≤1e-3）忽略。
 *   brakeFactor — 仅松手刹车（0~1，不含端点）。
 *                 默认 0.10；越大刹越快；不影响拖着跟手。非法值忽略。
 * 输入：viewer, sensitivity, brakeFactor
 * 返回：无
 */
void core_gl_viewer_set_orbit_feel(CoreGlViewer* viewer, float sensitivity,
                                   float brakeFactor);

/**
 * 功能：双指捏合缩放，改变相机到模型的距离。
 * 参数：
 *   viewer — 实例句柄；NULL 则空操作
 *   scale  — 捏合比例：>1 拉近（放大）；(0,1) 推远；≤0 忽略
 * 输入：viewer, scale
 * 返回：无
 */
void core_gl_viewer_zoom(CoreGlViewer* viewer, float scale);

/**
 * 功能：沿视线推拉相机（dolly）。
 * 参数：
 *   viewer — 实例句柄；NULL 则空操作
 *   amount — >0 靠近模型；<0 远离
 * 输入：viewer, amount
 * 返回：无
 */
void core_gl_viewer_dolly(CoreGlViewer* viewer, float amount);

/**
 * 功能：点光源沿世界 X 轴平移。
 * 参数：
 *   viewer — 实例句柄；NULL 则空操作
 *   amount — >0 向右；<0 向左
 * 输入：viewer, amount
 * 返回：无
 */
void core_gl_viewer_light_move_x(CoreGlViewer* viewer, float amount);

/**
 * 功能：点光源沿世界 Z 轴平移（控制灯时的前进/后退）。
 * 参数：
 *   viewer — 实例句柄；NULL 则空操作
 *   amount — >0 沿 +Z；<0 沿 -Z
 * 输入：viewer, amount
 * 返回：无
 */
void core_gl_viewer_light_move_z(CoreGlViewer* viewer, float amount);

/**
 * 功能：按屏幕风格 XY 增量拖动点光源（由 Pro 换算后传入）。
 * 参数：
 *   viewer — 实例句柄；NULL 则空操作
 *   dx     — 水平增量（与 orbit 同坐标系，右为正）
 *   dy     — 垂直增量（与 orbit 同坐标系，屏幕向下为正）
 * 输入：viewer, dx, dy
 * 返回：无
 */
void core_gl_viewer_light_drag(CoreGlViewer* viewer, float dx, float dy);

/**
 * 功能：开关平行光（太阳）。
 * 参数：enabled — 非 0 开；0 关
 */
void core_gl_viewer_set_dir_light(CoreGlViewer* viewer, int enabled);

/**
 * 功能：开关聚光灯（手电，跟相机朝向）。
 * 参数：enabled — 非 0 开；0 关
 */
void core_gl_viewer_set_spot_light(CoreGlViewer* viewer, int enabled);

/**
 * 功能：开关点光源（灯）。
 * 参数：enabled — 非 0 开；0 关
 */
void core_gl_viewer_set_point_light(CoreGlViewer* viewer, int enabled);

/**
 * 功能：设置点光源颜色（色卡，RGB 0~1）。
 */
void core_gl_viewer_set_point_light_color(CoreGlViewer* viewer, float r, float g, float b);

/**
 * 功能：相机左右平移位置（沿水平 Right）。
 * 参数：amount — >0 向右；<0 向左
 */
void core_gl_viewer_camera_pan(CoreGlViewer* viewer, float amount);

/**
 * 功能：爆炸视图（easeOut）。factor 为相对半径倍数，常用 1。
 */
void core_gl_viewer_explode(CoreGlViewer* viewer, float factor);

/**
 * 功能：爆炸复位（合拢）。
 */
void core_gl_viewer_implode(CoreGlViewer* viewer);

/**
 * 功能：屏幕点选取零件（像素坐标，左上为原点，与 UIKit 一致）。
 * 返回：命中 mesh 下标；未中 / 失败 → -1。选中件会高亮。
 */
int core_gl_viewer_pick(CoreGlViewer* viewer, float sx, float sy);

/**
 * 功能：当前选中 mesh 下标；无选中 → -1。
 */
int core_gl_viewer_selected(CoreGlViewer* viewer);

/**
 * 功能：清除选取高亮。
 */
void core_gl_viewer_clear_selection(CoreGlViewer* viewer);

/**
 * 功能：查询当前模型里有多少条动画 clip（load 后有效）。
 * 参数：
 *   viewer — 实例句柄；NULL 返回 0
 * 输入：viewer
 * 返回：clip 数量；无模型 / 无动画 → 0
 */
int core_gl_viewer_animation_count(CoreGlViewer* viewer);

/**
 * 功能：取第 index 条动画的 Assimp 源名字（供下拉显示）。
 * 参数：
 *   viewer — 实例句柄；NULL 返回 NULL
 *   index  — clip 下标，范围 [0, animation_count)
 * 输入：viewer, index
 * 返回：UTF-8 C 字符串指针（寿命随当前已 load 的 Model）；越界 / 无 → NULL
 */
const char* core_gl_viewer_animation_name(CoreGlViewer* viewer, int index);

/**
 * 功能：选中并播放第 index 条动画（用户点「开始」时调这个）。
 *       内部切到对应 Animation clip；之后每帧 render 里 Animator 插值骨矩阵。
 *       load 成功后默认已播 index 0（若有动画）。
 * 参数：
 *   viewer    — 实例句柄，不可为 NULL
 *   index     — clip 下标，范围 [0, animation_count)
 *   resetTime — 非 0：播放头回到 0；0：接着当前时间（少用）
 * 输入：viewer, index, resetTime
 * 返回：0 成功；-1 viewer 非法或 index 无对应 clip
 */
int core_gl_viewer_play_animation(CoreGlViewer* viewer, int index, int resetTime);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CORE_GL_VIEWER_H_ */
