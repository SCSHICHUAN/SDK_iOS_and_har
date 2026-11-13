package com.example.coresdk;

/**
 * Public JNI bridge shipped inside core.aar ({@code libcore_jni.so}).
 * GL APIs require the calling thread to have a current GLES3 context
 * (e.g. {@link android.opengl.GLSurfaceView} render thread).
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

  /** GLES3 context must be current. Returns opaque handle, or 0 on failure. */
  public static native long glViewerCreate();

  public static native void glViewerDestroy(long handle);

  /** Load model path ( Assimp ). Returns 0 on success. */
  public static native int glViewerLoad(long handle, String path);

  public static native void glViewerResize(long handle, int width, int height);

  public static native void glViewerRender(long handle);

  public static native void glViewerOrbit(long handle, float dx, float dy);

  /** Finger up — reseeds Arcball for the next drag. */
  public static native void glViewerOrbitEnd(long handle);

  /**
   * Orbit feel: gear ratio + residual damping.
   * Defaults: sensitivity=15, dampingFactor=0.04 (lower damping = heavier).
   */
  public static native void glViewerSetOrbitFeel(long handle, float sensitivity,
                                                float dampingFactor);

  /** Pinch: scale &gt; 1 zooms in. */
  public static native void glViewerZoom(long handle, float scale);

  /** amount &gt; 0 moves camera forward (closer). */
  public static native void glViewerDolly(long handle, float amount);

  /** amount &gt; 0 moves point light right. */
  public static native void glViewerLightMoveX(long handle, float amount);

  /** Drag point light with touch deltas. */
  public static native void glViewerLightDrag(long handle, float dx, float dy);
}
