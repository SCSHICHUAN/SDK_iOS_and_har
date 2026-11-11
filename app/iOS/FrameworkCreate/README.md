# FrameworkCreate

把共享静态库 `core.a` 打成 iOS `Core.xcframework` 的中间工程/脚本。

## 内容

- `Headers/`：umbrella `Core.h` + `module.modulemap`（Swift 可 `import Core`）
- `Info.plist`：framework 元数据
- `scripts/package_xcframework.sh`：device/sim `.a` → `Core.xcframework`
- `scripts/run_iospro.sh`：编译并启动 `release/iOS/iOSpro`
- `build/`：打包中间产物（可删）

## 一键入口

在仓库根目录或任意位置执行：

```bash
./app/core/scripts/build_ios.sh
```

流程：

1. toolchain 直接编 `.a` → `release/iOS/a/iphoneos` + `release/iOS/a/iphonesimulator`
2. 打 XCFramework → `release/iOS/iOSSDK/Core.xcframework`
3. 安装到 → `release/iOS/iOSpro/Frameworks/Core.xcframework`
4. 编译 `iOSpro` 并启动模拟器
