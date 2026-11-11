# Core SDK

## 核心架构

```
src/*.cpp
    +
app/core/CMake  +  (ios.toolchain / android.ndk / ohos.toolchain)
    =
  core.a   core.a   core.a          ← 三端同一套静态库
    |
    v
  XCFramework骨架   libcore_jni.so   libcore_napi.so
    |                 |                |
    v                 v                v
  Core.xcframework   core.aar         core.har     ← 可分发 SDK
    |                 |                |
    v                 v                v
  iOSpro           androidPro      harmonyOSpro    ← 集成验证
```

脚本对应哪一段（tree）：

```
app/
├── core/
│   ├── CMakeLists.txt
│   ├── cmake/toolchains/
│   │   ├── ios.toolchain.cmake          ← 编 iOS core.a
│   │   ├── android.toolchain.cmake      ← 编 Android core.a（薄包装 → include NDK 官方）
│   │   └── ohos.toolchain.cmake         ← 编 OHOS core.a
│   └── scripts/
│       ├── build_all_install.sh         ← 三端一键：.a → SDK → 装 Pro
│       ├── build_ios.sh                 ← 一键：.a → xcframework → iOSpro
│       ├── build_android.sh             ← 一键：.a → aar → androidPro
│       ├── build_ohos.sh                ← 一键：.a → har → harmonyOSpro
│       ├── build_assimp_android.sh      ← Assimp 源码 → android/<abi>/libassimp.a
│       ├── build_assimp_ohos.sh         ← Assimp 源码 → ohos/<arch>/libassimp.a
│       ├── build_host.sh                ← 本机编 .a 冒烟
│       └── clean_ios.sh                 ← 清 iOS 中间物 / DerivedData
│
├── iOS/FrameworkCreate/scripts/
│   ├── package_xcframework.sh           ← core.a → Core.xcframework
│   ├── install_iospro.sh                ← 覆盖 iOSSDK + 挂到 iOSpro
│   └── run_iospro.sh                    ← 编跑 iOSpro
│
├── Android/
│   ├── SoCreate/scripts/
│   │   └── package_so.sh                ← core.a → libcore_jni.so
│   └── CreateAAR/scripts/
│       ├── package_aar.sh               ← .so → core.aar（内调 package_so）
│       ├── install_androidpro.sh        ← 覆盖 AndroidSDK + 挂到 androidPro
│       └── run_androidpro.sh            ← 编跑 androidPro
│
└── HarmonyOS/
    └── CreateHAR/scripts/
        ├── package_har.sh               ← core.a → core.har（含 NAPI .so）
        ├── install_harmonypro.sh        ← 覆盖 HarmonySDK + 挂到 harmonyOSpro
        └── run_harmonypro.sh            ← 编跑 harmonyOSpro
```

对照架构图：

| 架构阶段 | iOS 脚本 | Android 脚本 | Harmony 脚本 |
|----------|----------|--------------|--------------|
| cpp+cmake → `.a` | `build_ios.sh` 前半 / ios.toolchain | `build_android.sh` 前半 / android.toolchain | `build_ohos.sh` 前半 / ohos.toolchain |
| `.a` → 中间体 | （装进 Framework 切片） | `SoCreate/package_so.sh` | （在 `package_har` 内链 NAPI） |
| 中间体 → SDK | `package_xcframework.sh` | `package_aar.sh` | `package_har.sh` |
| SDK → Pro | `install_iospro.sh` | `install_androidpro.sh` | `install_harmonypro.sh` |
| 跑 Pro | `run_iospro.sh` | `run_androidpro.sh` | `run_harmonypro.sh` |

一句话：

**C++ + CMake(+三端 toolchain) → 三个 `.a` →（Framework / `.so` / `.so`）→（XCFramework / AAR / HAR）→（iOSpro / androidPro / harmonyOSpro）**

| 阶段 | iOS | Android | HarmonyOS |
|------|-----|---------|-------------|
| 源码 | `src/` 同一份 | 同左 | 同左 |
| 编库工具 | `ios.toolchain` | `android.toolchain`（内部转调 NDK） | `ohos.toolchain` |
| 静态库 | `release/iOS/a/.../core.a` | `release/Android/a/.../core.a` | `release/HarmonyOS/a/.../core.a` |
| 中间体 | Framework 切片 | `libcore_jni.so` | `libcore_napi.so` |
| 分发 SDK | `Core.xcframework` | `core.aar` | `core.har` |
| Pro | `release/iOS/iOSpro` | `release/Android/androidPro` | `release/HarmonyOS/harmonyOSpro` |

- iOS：`.a` 直接装进 Framework → XCFramework（无打包工程）。
- Android / 鸿蒙：`.a` 先链进 `.so`（JNI / NAPI），再打进 AAR / HAR（有打包工程）。

目录对应：

```
src/                         核心 C/C++
app/core/                    CMake + toolchain + 一键脚本
app/iOS/FrameworkCreate/     .a → XCFramework
app/Android/SoCreate/        .a → .so
app/Android/CreateAAR/       .so → .aar
app/HarmonyOS/CreateHAR/     .a → .har（内含 NAPI）
release/<Platform>/{a, *SDK, *pro}/   ← 交付产物
debug/                       ← 构建日志 / 临时调试输出
```

---

## 使用

### 一键（改完 `src/` 就跑这个）

```bash
# 三端一键：编 .a → 打 SDK → 装进 Pro（默认不启动模拟器）
./app/core/scripts/build_all_install.sh
./app/core/scripts/build_all_install.sh arm64-v8a 24
RUN_PRO=1 ./app/core/scripts/build_all_install.sh   # 连 Pro 一起跑

# 单端
./app/core/scripts/build_ios.sh
./app/core/scripts/build_android.sh          # [abi=arm64-v8a] [api=24]
./app/core/scripts/build_ohos.sh             # [arch=arm64-v8a]
./app/core/scripts/build_host.sh             # 可选：本机冒烟
./app/core/scripts/clean_ios.sh              # 清 iOS 中间产物 / DerivedData
```

每条单端脚本都是同一四步：

`编 .a` → `打 SDK` → `覆盖进 Pro（并挂依赖）` → `编译运行`  
（`build_all_install` 默认 `SKIP_RUN=1`，只做到安装）

### 产物

| | `.a` | SDK | Pro |
|--|------|-----|-----|
| iOS | `release/iOS/a/{iphoneos,iphonesimulator}/core.a` | `release/iOS/iOSSDK/Core.xcframework` | `release/iOS/iOSpro` |
| Android | `release/Android/a/<abi>/core.a` | `release/Android/AndroidSDK/core.aar` | `release/Android/androidPro` |
| Harmony | `release/HarmonyOS/a/<arch>/core.a` | `release/HarmonyOS/HarmonySDK/core.har` | `release/HarmonyOS/harmonyOSpro` |

### API（`src/include/core/core.h`）

```c
int core_hello_world(void);
const char* core_hello_world_string(void);   // 勿 free
```

| Pro | 调用 |
|-----|------|
| iOSpro | `import Core` → `core_hello_world()` / `core_hello_world_string()` |
| androidPro | `CoreNative.helloWorld()` / `helloWorldString()` |
| harmonyOSpro | `@sdk/core` → `helloWorld()` / `helloWorldString()` |

---

## 脚本分工

入口只记 `app/core/scripts/build_*.sh`。底下按阶段拆：

```
build_*.sh
  │
  ├─ cmake(+toolchain) ──────────────► release/*/a/core.a
  ├─ package_*.sh ───────────────────► release/*/*SDK/
  │     iOS:    package_xcframework.sh
  │     Android: package_aar.sh → package_so.sh
  │     OHOS:   package_har.sh
  ├─ install_*.sh ───────────────────► 覆盖 SDK + Pro，挂工程依赖，清缓存
  └─ run_*.sh ───────────────────────► 只编 Pro 并装模拟器/设备
```

| 脚本 | 干什么 |
|------|--------|
| `build_all_install.sh` | 三端一键编到安装（默认不跑 Pro） |
| `build_{ios,android,ohos,host}.sh` | 单端一键全流程 |
| `clean_ios.sh` | 清 iOS 构建 / Xcode 缓存 |
| `package_xcframework.sh` / `package_aar.sh` / `package_har.sh` | `.a`（+桥）→ SDK |
| `package_so.sh`（Android SoCreate） | `.a` → `libcore_jni.so` |
| `install_{iospro,androidpro,harmonypro}.sh` | 强制覆盖 + 写入工程依赖 |
| `run_*.sh` | 只跑 Pro |

装 Pro 时不只拷文件：

- iOS：写/校验 `pbxproj` 链接 XCFramework，清 DerivedData  
- Android：确保 `include(":coresdk")` + `implementation(project(":coresdk"))`  
- 鸿蒙：写 `@sdk/core` file 依赖，清 `oh_modules` 后 `ohpm install`
