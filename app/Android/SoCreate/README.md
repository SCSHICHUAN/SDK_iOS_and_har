# SoCreate（底层 so 构建）

把 `core.a` 打成 `libcore_jni.so` 的底层工具。

**标准对外产物请走 `CreateAAR`**（对标 iOS XCFramework / HarmonyOS HAR）：

```bash
./app/core/scripts/build_android.sh arm64-v8a 24
```

`CreateAAR` 会产出 `release/Android/AndroidSDK/core.aar`（内含 JNI so + `com.example.coresdk.CoreNative`）。
本目录保留给需要单独拿 `.so` 的场景。
