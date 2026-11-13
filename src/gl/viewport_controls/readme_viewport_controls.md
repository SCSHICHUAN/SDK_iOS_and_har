# Viewport Controls — 选型 / 选轴 / Arcball / Touch XY

本文记录模型浏览手势的最终方案与取舍。实现见同目录：

| 文件 | 角色 |
|------|------|
| [`arcball.h`](arcball.h) | 完整轨迹球（球面 OA/OB → 任意轴 θ，四元数累加） |
| [`arcball_mobile.h`](arcball_mobile.h) | **当前 Pro 使用**：移动端浏览（Arcball 取 θ + 轴约束） |
| [`camera.h`](camera.h) | 固定观察相机；提供 `Right` / `Up` 给 mobile 选轴 |
| [`../renderer_data.cpp`](../renderer_data.cpp) | 枢轴：绕模型 AABB `center` 施加 `rotationMatrix()` |

---

## 1. 方案选型

| 方案 | 含义 | 结论 |
|------|------|------|
| 欧拉累加 yaw/pitch | 屏幕 δ 直接加角，易万向锁/手感差 | 已放弃作主路径 |
| 完整 Arcball | 球面两点 → 任意 3D 轴 + θ，可带 roll | 保留在 `arcball.h`，适合自由姿态 |
| **Mobile tumble（现行）** | Arcball **只算 θ**；轴钉死；begin/last 锁 dx 或 dy | **移动端看模型采用** |

**为何选 mobile 而不是纯 Arcball**

- 移动端看模型更常见「握住物体拧」：左右/上下分轴，少绕视线滚转。
- 完整 Arcball 斜拖会走大圆弧并带 roll，浏览手感容易「飘」。
- 仍用 Arcball 球面几何取角，保留跟手的非线性手感（球半径 / sensitivity）。

**对照**

```text
完整 Arcball:  拖拽 → OA/OB → n=OA×OB（任意方向）→ θ → 四元数
Mobile:        拖拽 → 锁成只用 dx 或 dy → OA/OB → θ(Arcball)
               → 轴强制为「物体 Y」或「相机 OX」→ 四元数
```

---

## 2. Arcball 内核（两边共用）

虚拟球心落在与**视口 / 相机 XY 平行**的平面上，半球朝向观察者（球局部 +Z）。

1. 屏幕点映射到单位半球 → 单位向量 **OA、OB**
2. **转轴方向**：`n = OA × OB`（再单位化；近平行则跳过）
3. **转角**：`cosθ = OA · OB` → `θ = acos(...)`（弧度）
4. **旋转矢量**：`(x,y,z,θ)` 或 `θ · n̂`；用 `angleAxis(θ, axis)` 进四元数

手感旋钮（不改 θ 数学本身）：

- `setViewport`：球半径 ≈ 视口短边一半
- `setSensitivity`：`r_eff = radius / sensitivity`（越大越灵敏）
- **同一 θ** 写入记录与 `angleAxis`，不再另乘裸增益

> 球心 ≠ 模型中心。球只负责交互取角；模型转动枢轴在 `RendererData`。

---

## 3. 选轴（Mobile 现行）

枢轴一律：**穿过模型 AABB 中心**（`translate(c) * R * translate(-c)`）。

| 手势 | 滚轴 | 施加方式 |
|------|------|----------|
| **左右**（用 dx） | **物体局部 +Y**（平行物体自身坐标系 Y） | 右乘 `Ry`：`orientation = orientation * angleAxis(±θ, (0,1,0))` |
| **上下**（用 dy） | **相机 OX**（`camera.Right`） | 左乘绕世界中该方向的轴 |

### 左右「翻到顶/底不反手」

绕物体局部 Y 时，上下翻到顶/底正对相机后，局部 Y 相对相机 Up 可能倒置，同向手指会像左右反了。

**取舍**

| 做法 | 说明 |
|------|------|
| A. 左右改绕相机/世界 Up | 最主流，永不反，但不是「绕物体自己的 Y」 |
| **B. 保留物体 Y + 符号修正（现行）** | `dot(localY, camUp) < 0` 时左右转角取反 |

现行选 **B**，兼顾「绕物体 Y」与手指方向一致。

---

## 4. Touch X / Y 怎么取舍

Pro 只传每帧增量 `(dx, dy)`；Core 在 `ArcballMobile` 内做锁定。

### 策略（现行）：begin vs last

```text
touch begin（抬手后第一帧）→ begin = (0,0)（相对手势原点）
每帧 MOVE                 → last += (dx, dy)
比较 |last.x - begin.x| 与 |last.y - begin.y|
  |Δx| ≥ |Δy|  → 本帧只用 dx（水平路径）
  |Δy| >  |Δx| → 本帧只用 dy（竖直路径）
抬手 endDrag              → 清空，下一笔重新 begin
```

**一帧不同时用 dx 和 dy**，避免斜拖合成斜轴。

### 为何不是「每帧比 |dx| vs |dy|」

| | 每帧瞬时 | begin vs last（现行） |
|--|----------|----------------------|
| 稳定性 | 斜拖时轴容易抖、来回切 | 手势主导方向锁定，更稳 |
| 观感 | 像在轴之间跳 | 更接近列表/地图的方向锁 |
| 主流 | 少 | 移动端手势更常见 |

### 与完整 Arcball 的输入差异

| | `arcball.h` | `arcball_mobile.h` |
|--|-------------|---------------------|
| 光标 | 可在球面上累计虚拟光标 | 每帧从**球极点**投射本帧单轴增量 |
| XY | 同时可用 dx、dy | begin/last 锁死后只喂一个分量 |
| 轴 | 完整 `OA×OB` | 钉死物体 Y 或相机 OX |

---

## 5. 数据流（现行）

```text
Pro 触摸
  DOWN/UP  → (UP) core_gl_viewer_orbit_end → ArcballMobile::endDrag
  MOVE     → orbit(dx,dy)
               → RendererData 刷新 camera.SetOrbit
               → ArcballMobile::drag(dx, dy, cam.Right, cam.Up)
               → begin/last 选 dx 或 dy
               → Arcball θ
               → 物体 Y 或 相机 OX 施加四元数

render
  model = T(center) * rotationMatrix() * T(-center)
  camera 固定观察；灯与相机不随物体转
```

---

## 6. 文件与切换

- **当前**：`RendererData` 使用 `ArcballMobile`。
- **对比完整轨迹球**：改回 `#include "arcball.h"` + `Arcball`，`drag` 签名无相机轴参数。

---

## 7. 一句话总结

> **Mobile 浏览 = Arcball 取 θ + begin/last 只选 dx 或 dy + 左右绕物体局部 Y（倒置符号修正）+ 上下绕相机 OX + 枢轴过模型中心。**  
> 完整 Arcball 留在 `arcball.h` 作自由姿态对照。
