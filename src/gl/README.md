# OpenGL（`src/gl`）

GLES 模型加载与浏览。模块对齐 LearnOpenGL / GL-ARKit：`Camera` / `Shader` / `Mesh` / `Model` / `RendererData`；对外 C API 见 `src/include/core/gl_viewer.h`；平台表面（FBO / present）在 Pro（如 iOS `ModelGLView`）。

编译：`CORE_ENABLE_GL=ON`（`build_ios.sh` / `build_android.sh` / `build_ohos.sh`）。

| 端 | 平台表面（MakeCurrent + FBO/present） | core 入口 |
|----|--------------------------------------|-----------|
| iOS | `ModelGLView`（EAGL + RBO present） | XCFramework `core_gl_viewer_*` |
| Android | `ModelGLSurfaceView`（`GLSurfaceView` 渲染线程已 current） | AAR JNI `CoreNative.glViewer*` |
| HarmonyOS | `XComponent` + native EGL（`ohos_gl_xcomponent.cpp`） | HAR NAPI + `glPresentAll` |

示例模型：`src/gl/models/cube.obj`。

---

## 标准调用流程

| 标准步骤 | 落点 | 对应 GL |
|----------|------|---------|
| 读模型（CPU） | `model.cpp` `loadModel` / `processNode` | Assimp → `Vertex` / indices |
| VBO / EBO / VAO | `mesh.h` `setupMesh`（load 时） | `glBufferData` / `glVertexAttribPointer` |
| layout 0/1/2 | `shaders/lighting-vs.vs` ↔ Attrib | `aPos` / `aNormal` / `aTexCoords` |
| 编译着色器 | `shader.h` + `shaders/lighting-*.vs\|fs` | `glCompileShader` / `glLinkProgram` |
| 清屏 + P/V/M + 光照 | `renderer_data.cpp` `render` | `glClear`；Dir/Point/Spot uniforms |
| use + Draw | `Model::Draw` → `Mesh::Draw` | `glUseProgram` / `glDrawElements` |
| 顶点 / 片元着色器 | `lighting-vs.vs` / `lighting-fs.fs`（AR `colors`） | Phong 多光源 + Material |
| 光栅化 | GPU 固定阶段 | （无代码） |
| Present | Pro（iOS：`presentRenderbuffer`） | 不在 core |

---

## 一帧绘制过程（重点：分片画进缓冲 → 叠起 → 最后 present）

### 为什么有 `Model::Draw` 的 for

```cpp
void Model::Draw(const Shader& shader) const {
  for (const auto& mesh : meshes) {
    mesh.Draw(shader);   // 每次 = 一片几何 glDrawElements
  }
}
```

一个模型常有多份 Mesh（不同部件 / 贴图）。**for = 分片绘制**：同一套 MVP/shader，换不同顶点（VAO），**一片一片画进当前帧缓冲（FBO）**，在缓冲里叠成完整画面；**不是**画到屏幕上当场闪一片。

```text
同一 FBO（缓冲）里：
  Draw mesh0 ──┐
  Draw mesh1 ──┼─→ 像素叠在一起（深度测试决定遮挡）
  Draw mesh2 ──┘
最后平台 present → 整张缓冲一次上屏
```

### Load 时（一般一次）

```text
Assimp → meshes[] → 每片 setupMesh（VBO/VAO/EBO、贴图）
```

只把数据放进 GPU，不上屏。

### 每帧

```text
① 平台：MakeCurrent + bind FBO          // 指定「画在缓冲里」
② core：clear → 设 MVP → Model::Draw
         for 分片 → 每片 glDrawElements  // 画在 FBO，屏幕仍是旧帧
③ 平台：presentRenderbuffer             // 缓冲 → 屏幕，整帧一次呈现
```

**一句话**：分片绘制发生在 **FBO 缓冲**；叠完再 **present**。人眼不会看到 for 循环里一片一片闪屏。

---

## 隐晦绑定：同一线程上的 Context 与 FBO

core **不接收** Context / FBO 句柄。平台在调用前把它们设成「当前」，core 里的 `gl*` 自动打到这套状态上。这是 GLES 的常态，不是漏传参。

两层「当前」叠在一起：

```text
线程
  └── current Context     ← MakeCurrent（隐晦绑到本线程）
        └── current FBO   ← glBindFramebuffer（隐晦绑到该 Context 的状态）
              └── glClear / Draw / glGen* 都作用在这里
```

### 1. GL Context（按线程）

某线程 `MakeCurrent(ctx)` 之后，该线程上所有 `gl*` 都进这个 Context；**不必把 Context 传进 core**。

```text
同一线程：
  平台 MakeCurrent(ctx)
  → core create / load / draw
  → VAO/VBO/EBO/纹理/Program 在创建瞬间归属该 Context
```

| 端 | 平台层 MakeCurrent |
|----|---------------------|
| iOS | `EAGLContext` + `setCurrentContext` |
| Android | `EGLContext` + `eglMakeCurrent` |
| HarmonyOS | EGL（或系统封装）+ MakeCurrent |

create / load / draw 须在 **同一线程、同一 Context**（或同 sharegroup）。换线程须在新线程再 MakeCurrent，否则 `glGen*` / Draw 无效或崩溃。

### 2. Framebuffer（按当前 Context 的状态）

FBO/RBO 由平台创建并挂好（color 常接窗口 Layer，depth 另挂）。画之前：

```text
glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer)
→ 再调 core_gl_viewer_render
→ core 只 glClear / Draw，不接 FBO id，写的就是这块当前 FBO
→ 平台再 present color RBO
```

和 Context 一样：不是把 `_framebuffer` 传进 SDK，而是 **bind 成当前 FBO**。没 bind 就 Draw，会画到默认 FBO / 错误目标，不是你的屏幕缓冲。

### 3. 一帧里平台要先做的两件事（iOS 例）

```text
ModelGLView.onTick
  setCurrentContext(_context)              // 隐晦：线程 ← Context
  glBindFramebuffer(..., _framebuffer)     // 隐晦：Context ← FBO
  core_gl_viewer_render(...)               // core 只用「当前」状态
  presentRenderbuffer(color RBO)           // 显示仍在平台层
```

**Present / Layer / 窗口表面** 留在 Pro，保证 core 三端可移植。

### 4. 只有这两个吗？

**对宿主 → core 的约定：主要就是这两层**（调用前由平台设好，且不把句柄传进 SDK）：

1. **current Context**（线程）  
2. **current 绘制 FBO**（该 Context 上）  

Context 上其实还有很多「当前绑定」，例如：

| 当前状态 | 谁来设 | 说明 |
|----------|--------|------|
| Program | core（`shader.use`） | `glUseProgram` |
| VAO | core（`Mesh::Draw`） | `glBindVertexArray` |
| Texture 单元 | core（`Mesh::Draw`） | `glActiveTexture` / `glBindTexture` |
| ARRAY_BUFFER 等 | core（`setupMesh` / Draw） | 上传或绘制时临时 bind |

这些 **不要求宿主隐晦准备**，是 core 在 load/render 路径里自己 bind 的。宿主若乱改同一 Context 上的状态，仍可能干扰绘制；约定是：**进 core 前后由平台保证 Context + FBO，其余 GL 状态交给 core。**
