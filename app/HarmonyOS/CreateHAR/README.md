# CreateHAR

把共享静态库 `core.a` 打成 HarmonyOS 可分发 `core.har`（对标 iOS `FrameworkCreate` → XCFramework）。

## 内容

- `project/core/`：HAR 模块（NAPI + ArkTS 导出）
- `scripts/package_har.sh`：`core.a` → `out/HarmonyOS/HarmonySDK/core.har`
- `scripts/run_harmonypro.sh`：编译并启动 `out/HarmonyOS/harmonyOSpro`

## 一键入口

```bash
./app/core/scripts/build_ohos.sh arm64-v8a
```

流程：

1. OHOS toolchain 编 `core.a` → `out/HarmonyOS/a/<arch>/`
2. 打 HAR → `out/HarmonyOS/HarmonySDK/core.har`
3. 安装到 → `harmonyOSpro`（`oh-package` 依赖 `file:../../HarmonySDK/core.har`）
4. 编译 HAP 并启动模拟器
