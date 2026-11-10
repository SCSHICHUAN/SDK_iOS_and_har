#include <jni.h>

#include "core/core.h"

extern "C" JNIEXPORT jint JNICALL
Java_com_example_coresdk_CoreNative_helloWorld(JNIEnv* /*env*/, jclass /*clazz*/) {
  return core_hello_world();
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_coresdk_CoreNative_helloWorldString(JNIEnv* env, jclass /*clazz*/) {
  const char* text = core_hello_world_string();
  return env->NewStringUTF(text != nullptr ? text : "");
}
