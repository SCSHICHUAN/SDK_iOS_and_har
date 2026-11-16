//
// gl_viewer.h — GLES 模型浏览器 C ABI（Swift / JNI / NAPI）
//
// 分层：本头文件 = 跨语言边界；RendererData = 一帧渲染；Pro = Surface/FBO。
// 约定：create/load/render 前须 makeCurrent GLES3；viewer==NULL 安全空操作。
//

#ifndef CORE_GL_VIEWER_H_
#define CORE_GL_VIEWER_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 不透明句柄。
 * 内部持有 RendererData（场景状态 + render）。
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
 *       内含 inertia.tick → arcball.drag、相机、灯光、Model::Draw。
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
 * 功能：按屏幕风格 XY 增量拖动点光源（由 Pro 换算后传入）。
 * 参数：
 *   viewer — 实例句柄；NULL 则空操作
 *   dx     — 水平增量（与 orbit 同坐标系，右为正）
 *   dy     — 垂直增量（与 orbit 同坐标系，屏幕向下为正）
 * 输入：viewer, dx, dy
 * 返回：无
 */
void core_gl_viewer_light_drag(CoreGlViewer* viewer, float dx, float dy);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CORE_GL_VIEWER_H_ */
