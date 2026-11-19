# OpenGL（`src/gl`）

GLES 模型加载与浏览。模块对齐 LearnOpenGL / GL-ARKit：`Camera` / `Shader` / `Mesh` / `Model` / `RendererData`；对外 C API 见 `src/include/core/gl_viewer.h`；平台表面（FBO / present）在 Pro（如 iOS `ModelGLView`）。

编译：`CORE_ENABLE_GL=ON`（`build_ios.sh` / `build_android.sh` / `build_ohos.sh`）。

| 端 | 平台表面（MakeCurrent + FBO/present） | core 入口 |
|----|--------------------------------------|-----------|
| iOS | `ModelGLView`（EAGL + RBO present） | XCFramework `core_gl_viewer_*` |
| Android | `ModelGLSurfaceView`（`GLSurfaceView` 渲染线程已 current） | AAR JNI `CoreNative.glViewer*` |
| HarmonyOS | `XComponent` + native EGL（`ohos_gl_xcomponent.cpp`） | HAR NAPI + `glPresentAll` |

示例模型：`src/gl/models/cube.obj`；iOSpro 另含 Wolf / SU7 / Benz / whiteMan / blackMan。

---

## 近两日开发摘要（2026-11-18 ~ 11-19）

按「职责拆开、Load 与 Play 分开、平台只管表面」落地 GLES 浏览器，并在 iOSpro 上验证多资源。

| 日期 | 做了什么 |
|------|----------|
| **11-18** | 拆清 `Model` / `Mesh` / `Animation` / `Animator` / `RendererData` / `ViewportInput` / `Render` / `CoreGlViewer`；Assimp 加载 → VAO；骨骼动画通路；弧球浏览；`DebutSequence` 入场（推近 + 绕 Y 自转，easeOutCubic） |
| **11-19** | 静态 glTF **节点 bake**（SU7 摆正）；**正面偏航** `preferredFrontYawOffset`；Benz 蒙皮车：**勿 cancel Sketchfab root**、绑定姿势包围盒、车辆不自动播门盖动画；车漆银灰兜底；头发跟资源色；**DisplayLink 生命周期**（退页必停）；骨骼矩阵 **按 palette 一次上传** |

资源对照：

| 资源 | 类型 | 要点 |
|------|------|------|
| cube.obj | 静态 | 基线 |
| Wolf FBX | 蒙皮 + 动画 | 世界缩放 `0.018`；FBX 重算 bone offset；`globalInverse` 取消 root |
| SU7 glb | 静态 glTF | 节点矩阵 bake（含 Z→Y）；正面 +π |
| Benz glb | **蒙皮** glTF（门盖动画） | root **不** cancel；默认停绑定姿势；车漆无 albedo → 银灰 |
| whiteMan / blackMan | 蒙皮人模 | 正面偏航 0；头发近黑无贴图 → 深棕兜底（与 AR 一致） |

---

## 架构怎么设计

### 设计原则

1. **按职责拆模块**：加载、存关键帧、播动画、存场景状态、改姿态、画一帧、C ABI 编排，各干一件事。
2. **顶点与动画 Load 时分开**：顶点进 `Mesh`（VAO）；clip 进 `Animation`（关键帧）。靠 **骨骼名字 / ID** 在 Draw 时汇合。
3. **core 不管窗口**：Context / FBO / present 留在 Pro；core 只认「当前 Context + 当前 FBO」。
4. **静态 vs 蒙皮两条路**：无骨 → bake 节点全局矩阵进顶点；有骨 → 顶点留局部空间，每帧乘骨矩阵。

### 分层（谁干什么）

```text
┌─────────────────────────────────────────────────────────┐
│  Pro（iOS ModelGLView / Android GLSurfaceView / OHOS）   │
│  MakeCurrent · bind FBO · CADisplayLink/帧回调 · present │
└──────────────────────────┬──────────────────────────────┘
                           │ C ABI
                           ▼
┌─────────────────────────────────────────────────────────┐
│  CoreGlViewer（gl_viewer.cpp）                            │
│  编排：load → Model；手势 → ViewportInput；               │
│        render → Animator::update + Render::frame         │
└───────┬─────────────┬──────────────┬────────────────────┘
        │             │              │
        ▼             ▼              ▼
   RendererData   Animator        Render
   数据袋          播 clip         清屏+MVP+灯+Draw
   model/shader/  → 骨矩阵表
   camera/灯/
   arcball/debut
        │
        ▼
   Model ──parse──► Mesh[]（VAO） + Animation[]（关键帧）
```

| 模块 | 文件 | 一句话 |
|------|------|--------|
| **Model** | `model.h/.cpp` | Assimp 读场景；静态 bake / 蒙皮权重；产出 Mesh + Animation |
| **Mesh** | `mesh.h` | `setupMesh` 上传 VBO/EBO/VAO；`Draw` 绑贴图 + 骨矩阵 + `glDrawElements` |
| **Animation** | `animation.h/.cpp` | 一条 clip 的 pos/rot/scale 关键帧；带 `boneInfoMap` 与 `globalInverse` |
| **Animator** | `animator.h/.cpp` | 沿骨树插值，写出 `globalBoneId → skinMatrix` |
| **RendererData** | `renderer_data.*` | 场景静态字段 + shader/灯 VAO 的 init/destroy |
| **ViewportInput** | `viewport_input.*` | 只改 Data（距离、弧球、灯、入场） |
| **Render** | `render.*` | 整帧：clear → debut/惯性 → MVP → 光照 → `Model::Draw` |
| **DebutSequence** | `debut_sequence.h` | 加载后不可打断的推近+自转，easeOutCubic 收尾 |
| **CoreGlViewer** | `Interface/gl_viewer.cpp` | 跨语言句柄，串起以上模块 |

### 数据流（Load 一次 / 每帧一次）

```text
【Load】
  Assimp::ReadFile
    → parseNode（递归节点）
        → processMesh
            · 无骨：Position = nodeGlobal * local   （bake 摆正/缩放）
            · 有骨：Position = local；写 BoneIDs/Weights + bonePalette
            · 贴图 / materialColor / isHair
            → Mesh 构造 → setupMesh → GPU VAO
    → loadAnimations → Animation[]（关键帧，不持 VAO）
    → computeBounds（蒙皮用绑定姿势骨矩阵算 AABB）
    → preferredFrontYawOffset / isVehicleAsset / preferredWorldScale

【每帧】
  Animator::updateAnimation(dt)     // 可 playing=false → 只算绑定姿势
    → finalBoneMatrices[globalId]
  Render::frame
    → Model::Draw(shader, &finalBoneMatrices)
        → Mesh::Draw：palette[local]=global[…] → setMat4Array → glDrawElements
```

---

## 怎么绘制 Model（原理）

### 为什么有多片 Mesh

一个 glTF/FBX 常有车身、轮毂、玻璃、内饰等多份几何，材质/贴图不同。`Model::Draw` 的 for **不是**「一片一片闪到屏幕」，而是 **同一 FBO 里分片叠画**，深度测试决定遮挡，最后 Pro `present` 一次上屏。

```text
同一 FBO：
  mesh0 车漆 ──┐
  mesh1 轮胎 ──┼─→ 像素叠在缓冲里
  mesh2 玻璃 ──┘
present → 整帧上屏
```

### 静态模型（SU7、部分零件）

- 节点链上常有 Sketchfab **Z-up → Y-up** 的 root 旋转，以及局部缩放。
- Load 时把 `nodeGlobal` **bake 进顶点**，之后按普通 Mesh 画，不再依赖骨矩阵。
- `preferredFrontYawOffset = π`：资源「车头」常背对相机，入场结束角 = π + 45°。

### 蒙皮模型（Wolf、人模、Benz）

顶点着色器（`lighting-vs.vs`）逻辑：

```text
若顶点有骨权重：
  worldPos = Σ weight_i * finalBonesMatrices[boneId_i] * localPos
否则：
  worldPos = localPos
再乘 model / view / projection
```

要点：

- **骨 ID 只在 Load 时写入 VBO**；每帧只换矩阵表，不重传顶点。
- 每 Mesh 有 **bonePalette**：局部槽 → 全局骨 id（shader 槽位上限 100，见 `aiProcess_SplitByBoneCount`）。
- 上传用 **`setMat4Array` 一次传实际槽数**，禁止对每个 mesh 循环 100 次 `glGetUniformLocation`（Benz ~500 mesh 会直接打烫手机）。

### 材质色与贴图

- 优先 `BASE_COLOR` / `DIFFUSE` / `EMISSIVE`；**不要**对普通材质乱用 `UNKNOWN`（常为 metallicRoughness，会把 SU7 车身弄花）。
- 无 diffuse 时用白 fallback × `materialColor`（glTF `baseColorFactor`）。
- **车漆**（名含 `carpaint`）无 albedo 且 factor 近白 → 银灰兜底（Benz 资源本身无车漆贴图）。
- **头发**近黑无贴图 → 深棕兜底；有贴图则用资源色，不臆造金发。

### 浏览与入场

- **ArcballMobile**：指尖拖旋转模型；θ 跟屏高走。
- **Camera 轨道**：距离 / yaw / pitch；FBX 固定参考距离，其它按包围球 `fitDistance`。
- **DebutSequence**：load 后先推近并绕 Y 转一圈收在「正面+45°」；进行中手势不打断；结束后可弧球。

---

## 动画原理

### 角色分工

| | Animation | Animator |
|--|-----------|----------|
| 时机 | Load | 每帧 |
| 存什么 | 某 clip 各骨的关键帧 | 当前时间 + 结果矩阵表 |
| 不做 | 不建 VAO、不推进时间 | 不读文件 |

### 蒙皮公式

```text
skinMatrix = globalInverse * boneGlobalTransform * inverseBindMatrix(offset)
```

- `boneGlobalTransform`：根到该骨的累积（无通道用绑定节点变换；有通道用插值 TRS）。
- `offset`：Assimp `mOffsetMatrix`（glTF 的 inverseBindMatrices）；**FBX 常错**，故 Wolf 在挂 clip 前用绑定姿势 **重算** `offset = inv(boneGlobal) * meshGlobal`。
- `globalInverse`：
  - **FBX**：`inv(root)`，把厂商根变换拉回模型空间。
  - **Sketchfab glTF**：**必须用单位阵**。root 上的旋转是 Z→Y 摆正；再取逆会 **整车头朝下、轮子错位**（Benz 踩过的坑）。

### 车辆动画

Benz 带门/盖/雨刮等 clip。默认：

- `isVehicleAsset()`（材质/动画名含 carpaint、door、hood、trunk…）→ `Animator::setPlaying(false)`，每帧仍算 **绑定姿势** 矩阵，门不自动开合。
- 用户点 Anim 下拉 → `setPlaying(true)` 再播。

人模 / 狼：load 后默认循环播 clip 0。

### 时间轴

```text
currentTime += ticksPerSecond * dt
有通道的骨：插值 T/R/S → localMatrix
无通道的骨：用 aiNode::mTransformation（绑定姿势）
沿树：global = parent * local
写入 finalBoneMatrices[id]
```

---

## 生命周期与发热（务必遵守）

| 问题 | 原因 | 做法 |
|------|------|------|
| 退浏览页后手机发烫 | `CADisplayLink` **强引用** View，退页后仍 60fps | `viewWillDisappear` → `pauseRendering`（invalidate）；`deinit` → `shutdown` |
| Benz 浏览时过烫 | 每 mesh 上传 100 骨 × `glGetUniformLocation` | `setMat4Array` 只传 palette 长度 |
| 反复进退内存涨 | destroy 不彻底 / 大 glb 反复拷贝 | `Model::destroy` 清顶点+删纹理+reset Assimp；Documents 已有同尺寸 glb 则跳过拷贝 |

平台约定：

```text
进页：resumeRendering（或 didMoveToWindow 有 window）
离页：pauseRendering（打断 DisplayLink 引用环）
销毁：shutdown → core_gl_viewer_destroy → Model/Shader/FBO 全释放
```

core 侧 `Model::destroy` 顺序：先 `delete` Animation → 清 mesh CPU/VAO → `glDeleteTextures` → `importer_.reset()`。

---

## 标准调用流程（速查）

| 步骤 | 落点 | 对应 GL |
|------|------|---------|
| 读模型（CPU） | `model.cpp` Parse + 骨权重 | Assimp → Vertex / indices / BoneIDs |
| VBO / EBO / VAO | `mesh.h` `setupMesh`（load 时） | layout 0/1/2 + **5/6 骨** |
| 挂动画 clip | `Model::loadAnimations` | `Animation` 关键帧 |
| 每帧插值 | `Animator::updateAnimation` | CPU 骨矩阵表 |
| 清屏 + P/V/M + 光照 | `Render::frame` | `glClear`；Dir/Point/Spot |
| use + Draw | `Model::Draw` → `Mesh::Draw` | 绑 VAO + `finalBonesMatrices` |
| Present | Pro | 不在 core |

### 隐晦绑定：同一线程上的 Context 与 FBO

core **不接收** Context / FBO 句柄。平台在调用前设成「当前」，core 里的 `gl*` 打到这套状态上。

```text
线程
  └── current Context     ← MakeCurrent
        └── current FBO   ← glBindFramebuffer
              └── glClear / Draw / glGen* 都作用在这里
```

| 端 | MakeCurrent |
|----|-------------|
| iOS | `EAGLContext` + `setCurrentContext` |
| Android | `EGLContext` + `eglMakeCurrent` |
| HarmonyOS | EGL + MakeCurrent |

create / load / draw 须在 **同一线程、同一 Context**。

iOS 一帧例：

```text
ModelGLView.onTick
  setCurrentContext(_context)
  glBindFramebuffer(..., _framebuffer)
  core_gl_viewer_render(...)     // 内含 Animator + Render::frame
  presentRenderbuffer(color RBO)
```

---

## 关键文件索引

```text
src/gl/
  model.{h,cpp}           加载 / bake / 包围盒 / 车辆检测
  mesh.h                  VAO 与 Draw
  animation.{h,cpp}       clip 关键帧 + globalInverse 策略
  animator.{h,cpp}        播放 / playing 开关
  render.{h,cpp}          整帧
  renderer_data.{h,cpp}   数据袋
  viewport_input.{h,cpp}  手势与 load 后布局
  debut_sequence.h        入场
  shaders/lighting-*.{vs,fs}
  Interface/gl_viewer.cpp C ABI 编排
src/include/core/gl_viewer.h
```
