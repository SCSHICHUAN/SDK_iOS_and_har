# Viewport Controls — 选型 / 选轴 / Arcball / 残差阻尼

本文记录模型浏览手势的最终方案与取舍。实现见同目录：

| 文件 | 角色 |
|------|------|
| [`arcball.h`](arcball.h) | 完整轨迹球（球面 OA/OB → 任意轴 θ，四元数累加） |
| [`arcball_mobile.h`](arcball_mobile.h) | **当前 Pro 使用**：局部 Y + 相机 OX + 齿轮灵敏度 |
| [`Inertia.h`](Inertia.h) | **残差阻尼**（拖/松同一条曲线，对标 Sketchfab / su7） |
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
- 幅度：`θ = (px / radius) * sensitivity`（齿轮比，可超过跟手）。

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

## 3. 残差阻尼 Inertia（现行手感核心）

对标 Sketchfab / `su7.mov`：**不是立刻消费输入，而是有消费速度。**

```text
move：  residual += (dx, dy)          ← 只记账，不转模型
每帧：  apply = residual × d
        residual -= apply
        arcball.drag(apply, …)       ← 真正转
end：   残差继续泄                    ← 自然滑行（不必另估速）
```

`d = dampingFactor`（指数衰减）。拖着按住不动也会「追上」残差 → 沉；松手同一条曲线。

| 操作 | 观感 |
|------|------|
| 突然快划 | 残差堆高，多帧才追上 → 厚重 |
| 匀速拖 | 稳态跟手，仍略滞后 |
| 甩完松手 | 残差未清完 → 软滑行 |
| 慢慢停再松 | 残差≈0 → 几乎不滑 |

**旧方案**（已弃）：拖时立刻 `drag(dx)`，松手再 `v=Δs/Δt` coast —— 拖的时候没有惯性，对不上 su7。

---

## 4. 默认值与对外调参

| 参数 | 默认 | 含义 |
|------|------|------|
| **sensitivity**（比例/齿轮比） | **15** | 同样像素转多少角；↑ 更灵敏 |
| **dampingFactor**（惯性消费速度） | **0.04** | 每帧吃掉残差的比例；**↓ 更沉、滑更久**；↑ 更跟手 |

```c
// C / 各端统一入口
core_gl_viewer_set_orbit_feel(viewer, /*sensitivity*/ 15.f, /*dampingFactor*/ 0.04f);

// 更沉一点
core_gl_viewer_set_orbit_feel(viewer, 15.f, 0.025f);

// 转得更多 + 稍跟手
core_gl_viewer_set_orbit_feel(viewer, 25.f, 0.06f);
```

`RendererData::init()` 会设成上述默认；之后可随时 `set_orbit_feel` 覆盖。  
**不要**用把 `sensitivity` 拉很大来冒充厚重——厚重只调 `dampingFactor`。

---

## 5. 数据流（现行）

```text
Pro 触摸
  MOVE  → core_gl_viewer_orbit(dx,dy)
            → inertia_.move          // 累残差
  UP    → core_gl_viewer_orbit_end
            → inertia_.end           // 残差继续泄

render  → inertia_.tick → (applyDx, applyDy)
            → ArcballMobile::drag(apply, cam.Right, cam.Up)
            → model = T(c) * R * T(-c)
```

---

## 6. 文件与切换

- **当前**：`RendererData` = `ArcballMobile` + `Inertia` 残差阻尼。
- **对比完整轨迹球**：改回 `#include "arcball.h"` + `Arcball`（无相机轴参数）。

三端 touch **无加速度**；不用系统加速度计。

---

## 7. 一句话总结

> **Mobile 浏览 = 局部 Y（倒置修正）+ 相机 OX + 齿轮 θ + 枢轴过中心 + 残差阻尼（输入不立即消费，按 dampingFactor 消费；拖/松同一条曲线）。**  
> 对外：`core_gl_viewer_set_orbit_feel(sensitivity=15, dampingFactor=0.04)`。
