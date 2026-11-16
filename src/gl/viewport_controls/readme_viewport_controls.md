# Viewport Controls — 选型 / 选轴 / Arcball / 残差阻尼

本文记录模型浏览手势的最终方案与取舍。实现见同目录。

**头文件规范（对齐 [`camera.h`](camera.h)）**：类外 `kXxx` 默认常量 → `public` 里先 `// —— members ——`（PascalCase 公开字段）再 `// —— methods ——` → `private` 只放辅助。改手感优先改常量 / 公开成员，而不是翻方法实现。

| 文件 | 角色 |
|------|------|
| [`arcball.h`](arcball.h) | 完整轨迹球（球面 OA/OB → 任意轴 θ，四元数累加） |
| [`arcball_mobile.h`](arcball_mobile.h) | **当前 Pro 使用**：局部 Y + 相机 OX + 齿轮灵敏度 |
| [`inertia.h`](inertia.h) | 拖动跟手 + 独立松手刹车 |
| [`camera.h`](camera.h) | 固定观察相机；提供 `Right` / `Up` 给 mobile 选轴 |
| [`../renderer_data.cpp`](../renderer_data.cpp) | 接线：move→残差，tick→drag；枢轴过 AABB center |

对外 API：`core_gl_viewer_set_orbit_feel(viewer, sensitivity, dampingFactor)`  
（Android `glViewerSetOrbitFeel` / Harmony `glViewerSetOrbitFeel`）

---

## 1. 方案选型

| 方案 | 含义 | 结论 |
|------|------|------|
| 欧拉累加 yaw/pitch | 屏幕 δ 直接加角，易万向锁/手感差 | 已放弃作主路径 |
| 完整 Arcball | 球面两点 → 任意 3D 轴 + θ，可带 roll | 保留在 `arcball.h`，适合自由姿态 |
| **Mobile tumble（现行）** | 符号用 Arcball；dx→物体局部 Y、dy→相机 OX；幅度用齿轮比 | **移动端看模型采用** |

**为何选 mobile 而不是纯 Arcball**

- 移动端看模型更常见「握住物体拧」：左右/上下分轴，少绕视线滚转。
- 完整 Arcball 斜拖会走大圆弧并带 roll，浏览手感容易「飘」。
- 幅度对齐 OrbitControls：`θ = 2π · (px / height) · sensitivity`（默认 1 ≈ 一屏高一圈）。

```text
完整 Arcball:  拖拽 → OA/OB → n=OA×OB（任意方向）→ θ → 四元数
Mobile:        dx→物体局部 Y（倒置符号修正），dy→相机 OX；θ 用齿轮比
```

---

## 2. 选轴（Mobile 现行）

枢轴一律：**穿过模型 AABB 中心**（`translate(c) * R * translate(-c)`）。

| 手势 | 滚轴 | 施加方式 |
|------|------|----------|
| **左右**（dx） | **物体局部 +Y** | 右乘 `Ry`；`dot(localY, camUp)<0` 时符号取反 |
| **上下**（dy） | **相机 OX** | 左乘绕该轴 |

手感（灵敏度 / 阻尼）与选轴分开调，**不要为了手感改转轴**。

Touch **不锁轴**：`(dx,dy)` 可同时非零，一帧内分别施加。

---

## 3. Inertia：拖动比例 ⊥ 松手刹车（必须解耦）

旧残差阻尼把「拖着追残差」和「松手滑行」绑在同一个 `d` 上 → 一改刹车就影响拖动。现行拆开：

```text
③ 拖动比例（sensitivity = OrbitControls rotateSpeed）
   手指 dx → pending 100% → tick 全量吐出 → drag
   θ = 2π · (px / height) · sensitivity；与刹车无关

④ 松手刹车（brakeFactor）
   end：近窗估速 → coast；tick：v·dt 后 v*=(1-brake)
   与拖动比例无关
```

| 参数 | 管什么 | 不管什么 |
|------|--------|----------|
| `sensitivity` | 拖多少 → 转多少 | 松手滑多久 |
| `brakeFactor` | 松手刹多快 | 拖着转多少 |

启动：`begin` 只打断 coast、清速度，不再另乘启动增益。

---

## 4. 默认值（对标 Sketchfab / su7）

| 参数 | 默认 | 含义 |
|------|------|------|
| **sensitivity** | **1.0** | 一屏高 ≈ 转一圈 |
| **brakeFactor** | **0.10** | 松手约 0.5s 软刹 |

```c
core_gl_viewer_set_orbit_feel(viewer, /*sensitivity*/ 1.f, /*brakeFactor*/ 0.10f);

// 拖得更猛，刹车不变
core_gl_viewer_set_orbit_feel(viewer, 1.4f, 0.10f);

// 拖动不变，刹得更干脆
core_gl_viewer_set_orbit_feel(viewer, 1.f, 0.16f);
```

---

## 5. 数据流（现行）

```text
Pro 触摸
  MOVE  → orbit(dx,dy) → inertia_.move   // pending += dx（100% 跟手）+ 采样速度
  UP    → orbit_end    → inertia_.end    // 独立估速启动刹车

render  → inertia_.tick
            → 吐出 pending（拖动）
            → 若 coast：叠加 v·dt 并 brake
            → ArcballMobile::drag(...)
```

---

## 6. 文件与切换

- **当前**：`ArcballMobile`（比例）+ `Inertia`（仅松手刹车）。
- **对比完整轨迹球**：改回 `#include "arcball.h"` + `Arcball`。

三端 touch **无加速度**；不用系统加速度计。

---

## 7. 一句话总结

> **Mobile 浏览 = 局部 Y + 相机 OX + OrbitControls 幅度(sensitivity=1) + 枢轴过中心 + 松手独立刹车(brakeFactor=0.10)。**  
> 对外：`core_gl_viewer_set_orbit_feel(1.0, 0.10)`。
