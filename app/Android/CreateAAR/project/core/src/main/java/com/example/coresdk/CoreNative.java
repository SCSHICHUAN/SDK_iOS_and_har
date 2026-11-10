package com.example.coresdk;

/**
 * Public JNI bridge shipped inside core.aar ({@code libcore_jni.so}).
 */
public final class CoreNative {
  static {
    System.loadLibrary("core_jni");
  }

  private CoreNative() {}

  /** Calls core_hello_world(); returns 0 on success. */
  public static native int helloWorld();

  /** Calls core_hello_world_string(); returns "Hello World!". */
  public static native String helloWorldString();
}
