# SoCreate（底层 so 构建）

把 `core.a` 打成 `libcore_napi.so` 的底层工具。

**标准对外产物请走 `CreateHAR`**（对标 iOS XCFramework）：

```bash
./app/core/scripts/build_ohos.sh arm64-v8a
```

`CreateHAR` 会在 HAR 模块内完成 NAPI 编译并产出 `release/HarmonyOS/HarmonySDK/core.har`。
本目录保留给需要单独拿 `.so` 的场景。
