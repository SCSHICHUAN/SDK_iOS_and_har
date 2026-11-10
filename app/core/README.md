# app/core

编共享静态库 `core.a` 的 CMake 工程与**一键流水线入口脚本**。

完整架构、使用方式与脚本职责见仓库根目录：

**[../../README.md](../../README.md)**

## 本目录脚本

| 脚本 | 作用 |
|------|------|
| `scripts/build_all_install.sh` | 三端一键：编到安装（默认不跑 Pro） |
| `scripts/build_host.sh` | 本机构建冒烟 |
| `scripts/build_ios.sh` | iOS 全流程 |
| `scripts/build_android.sh` | Android 全流程 |
| `scripts/build_ohos.sh` | HarmonyOS 全流程 |
| `scripts/clean_ios.sh` | 清理 iOS 构建与 Xcode 缓存 |
