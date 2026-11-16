//
// gl_platform.h — unified GLES3 headers for iOS / Android / HarmonyOS
// 各端 GLES3 头文件统一入口
//
// Aligned with GL-ARKit/ARKit/GL/gl_platform.h; extended for Android / OHOS.
// 对齐 GL-ARKit 的 gl_platform.h，并扩展 Android / HarmonyOS。
//
// Usage / 用法：
//   Core code `#include "gl_platform.h"` to get gl*.
//   core 内部 `#include "gl_platform.h"` 即可拿到 gl*。
//   Do not include <GLES3/gl3.h> directly in app code (ABI differs by OS).
//   不要在业务里直接 `#include <GLES3/gl3.h>`，以免端差异。
//
// Branches / 分支：
//   iOS           → OpenGLES/ES3/gl.h (+ glext), silence deprecation
//                   OpenGLES/ES3，并压 GLES 弃用警告
//   Android / OHOS → GLES3/gl3.h
//   other          → compile error (mobile GLES3 only)
//                   编译错误（本 SDK 只支持移动 GLES3）
//

#ifndef CORE_GL_PLATFORM_H_
#define CORE_GL_PLATFORM_H_

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
#ifndef GLES_SILENCE_DEPRECATION
#define GLES_SILENCE_DEPRECATION
#endif
#include <OpenGLES/ES3/gl.h>
#include <OpenGLES/ES3/glext.h>
#elif defined(__ANDROID__) || defined(ANDROID) || defined(__OHOS__)
#include <GLES3/gl3.h>
#else
#error "core GL: unsupported platform (need iOS / Android / OHOS GLES3)"
#endif

#endif /* CORE_GL_PLATFORM_H_ */
