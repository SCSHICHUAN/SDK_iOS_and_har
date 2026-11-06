# core SDK

跨平台 C/C++ 静态库，产物为 `core.a`。当前提供基础能力：打印 `Hello World!`。

支持平台：Host / iOS / Android / HarmonyOS。

## 仓库目录

```
src/                         # 源码
  include/core/core.h        # 对外 C API
  core.cpp                   # C++ 实现
  examples/hello_main.c      # 本机冒烟示例
app/
  core/                      # 编 core.a 的 CMake / toolchain / scripts
  Android|iOS|HarmonyOS/     # 各端 Demo App（后续）
out/
  core/<platform>/           # core.a 等构建产物
  Android|iOS|HarmonyOS/     # 各端 App 产物（后续）
```

## API

```c
#include "core/core.h"

int core_hello_world(void);  /* 打印 Hello World!，成功返回 0 */
```

## 本机构建

```bash
./app/core/scripts/build_host.sh
# 产物: out/core/host/lib/core.a
./out/core/host/core_hello
```

## Android

```bash
./app/core/scripts/build_android.sh arm64-v8a 24
# 产物: out/core/android-arm64-v8a/lib/core.a
```

## iOS

```bash
./app/core/scripts/build_ios.sh OS64
./app/core/scripts/build_ios.sh SIMULATORARM64
```

## HarmonyOS

```bash
./app/core/scripts/build_ohos.sh arm64-v8a
```
