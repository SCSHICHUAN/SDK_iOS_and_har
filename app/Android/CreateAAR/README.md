# CreateAAR

把共享静态库 `core.a` 打成 Android 可分发 `core.aar`（对标 iOS XCFramework / HarmonyOS HAR）。

## 内容

- `project/core/`：Android Library（JNI + Kotlin API）
- `scripts/package_aar.sh`：`core.a` → `out/Android/AndroidSDK/core.aar`
- `scripts/run_androidpro.sh`：编译并启动 `out/Android/androidPro`

## 一键入口

```bash
./app/core/scripts/build_android.sh arm64-v8a 24
```

流程：

1. NDK 编 `core.a` → `out/Android/a/<abi>/`
2. 打 AAR → `out/Android/AndroidSDK/core.aar`
3. 安装到 → `androidPro`（`implementation(files(.../core.aar))`）
4. Gradle 编 APK 并启动模拟器
