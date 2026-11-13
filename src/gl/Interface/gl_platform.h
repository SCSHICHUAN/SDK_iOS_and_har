//
// gl_platform.h — OpenGL ES 3 headers for iOS / Android / HarmonyOS
// Aligned with GL-ARKit/ARKit/GL/gl_platform.h, extended for mobile ABIs.
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
