#include <jni.h>

#include "core/core.h"
#include "core/gl_viewer.h"

extern "C" JNIEXPORT jint JNICALL
Java_com_example_coresdk_CoreNative_helloWorld(JNIEnv* /*env*/, jclass /*clazz*/) {
  return core_hello_world();
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_coresdk_CoreNative_helloWorldString(JNIEnv* env, jclass /*clazz*/) {
  const char* text = core_hello_world_string();
  return env->NewStringUTF(text != nullptr ? text : "");
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_example_coresdk_CoreNative_glViewerCreate(JNIEnv* /*env*/, jclass /*clazz*/) {
  return reinterpret_cast<jlong>(core_gl_viewer_create());
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_coresdk_CoreNative_glViewerDestroy(JNIEnv* /*env*/, jclass /*clazz*/, jlong handle) {
  core_gl_viewer_destroy(reinterpret_cast<CoreGlViewer*>(handle));
}

extern "C" JNIEXPORT jint JNICALL
Java_com_example_coresdk_CoreNative_glViewerLoad(JNIEnv* env, jclass /*clazz*/, jlong handle,
                                                 jstring path) {
  if (!handle || !path) return -1;
  const char* cpath = env->GetStringUTFChars(path, nullptr);
  if (!cpath) return -1;
  const int rc = core_gl_viewer_load(reinterpret_cast<CoreGlViewer*>(handle), cpath);
  env->ReleaseStringUTFChars(path, cpath);
  return rc;
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_coresdk_CoreNative_glViewerResize(JNIEnv* /*env*/, jclass /*clazz*/, jlong handle,
                                                   jint width, jint height) {
  core_gl_viewer_resize(reinterpret_cast<CoreGlViewer*>(handle), width, height);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_coresdk_CoreNative_glViewerRender(JNIEnv* /*env*/, jclass /*clazz*/, jlong handle) {
  core_gl_viewer_render(reinterpret_cast<CoreGlViewer*>(handle));
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_coresdk_CoreNative_glViewerOrbit(JNIEnv* /*env*/, jclass /*clazz*/, jlong handle,
                                                  jfloat dx, jfloat dy) {
  core_gl_viewer_orbit(reinterpret_cast<CoreGlViewer*>(handle), dx, dy);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_coresdk_CoreNative_glViewerOrbitEnd(JNIEnv* /*env*/, jclass /*clazz*/,
                                                     jlong handle) {
  core_gl_viewer_orbit_end(reinterpret_cast<CoreGlViewer*>(handle));
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_coresdk_CoreNative_glViewerZoom(JNIEnv* /*env*/, jclass /*clazz*/, jlong handle,
                                                 jfloat scale) {
  core_gl_viewer_zoom(reinterpret_cast<CoreGlViewer*>(handle), scale);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_coresdk_CoreNative_glViewerDolly(JNIEnv* /*env*/, jclass /*clazz*/, jlong handle,
                                                  jfloat amount) {
  core_gl_viewer_dolly(reinterpret_cast<CoreGlViewer*>(handle), amount);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_coresdk_CoreNative_glViewerLightMoveX(JNIEnv* /*env*/, jclass /*clazz*/,
                                                       jlong handle, jfloat amount) {
  core_gl_viewer_light_move_x(reinterpret_cast<CoreGlViewer*>(handle), amount);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_coresdk_CoreNative_glViewerLightDrag(JNIEnv* /*env*/, jclass /*clazz*/,
                                                      jlong handle, jfloat dx, jfloat dy) {
  core_gl_viewer_light_drag(reinterpret_cast<CoreGlViewer*>(handle), dx, dy);
}
