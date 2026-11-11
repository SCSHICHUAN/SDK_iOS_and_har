package com.example.androidpro

import android.content.Context
import android.opengl.GLSurfaceView
import android.util.AttributeSet
import android.view.MotionEvent
import android.view.ScaleGestureDetector
import com.example.coresdk.CoreNative
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10
import kotlin.math.max

/**
 * Platform surface (≈ iOS ModelGLView / AR SCRenderer):
 * GLSurfaceView makes GLES context current on the GL thread before callbacks;
 * core only draws into the current framebuffer.
 */
class ModelGLSurfaceView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
) : GLSurfaceView(context, attrs) {

    private val renderer = ModelRenderer(context)
    private var lastX = 0f
    private var lastY = 0f
    /** false = drag rotates box; true = drag moves point light. */
    @Volatile var dragControlsLight: Boolean = false
    private val scaleDetector = ScaleGestureDetector(
        context,
        object : ScaleGestureDetector.SimpleOnScaleGestureListener() {
            override fun onScale(detector: ScaleGestureDetector): Boolean {
                queueEvent { renderer.zoom(detector.scaleFactor) }
                return true
            }
        },
    )

    init {
        setEGLContextClientVersion(3)
        setRenderer(renderer)
        renderMode = RENDERMODE_CONTINUOUSLY
    }

    fun setModelPath(path: String) {
        queueEvent { renderer.setModelPath(path) }
    }

    fun dolly(amount: Float) {
        queueEvent { renderer.dolly(amount) }
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        scaleDetector.onTouchEvent(event)
        if (scaleDetector.isInProgress) return true
        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN -> {
                lastX = event.x
                lastY = event.y
            }
            MotionEvent.ACTION_MOVE -> {
                val dx = event.x - lastX
                val dy = event.y - lastY
                lastX = event.x
                lastY = event.y
                // Object: (dx, dy). Light: flip Y only → (dx, -dy).
                if (dragControlsLight) {
                    queueEvent { renderer.lightDrag(dx, -dy) }
                } else {
                    queueEvent { renderer.orbit(dx, dy) }
                }
            }
        }
        return true
    }

    private class ModelRenderer(private val appContext: Context) : Renderer {
        private var handle: Long = 0
        private var modelPath: String? = null
        private var pendingPath: String? = null

        fun setModelPath(path: String) {
            pendingPath = path
            if (handle != 0L) {
                loadPending()
            }
        }

        fun orbit(dx: Float, dy: Float) {
            if (handle != 0L) CoreNative.glViewerOrbit(handle, dx, dy)
        }

        fun lightDrag(dx: Float, dy: Float) {
            if (handle != 0L) CoreNative.glViewerLightDrag(handle, dx, dy)
        }

        fun zoom(scale: Float) {
            if (handle != 0L) CoreNative.glViewerZoom(handle, max(scale, 0.0001f))
        }

        fun dolly(amount: Float) {
            if (handle != 0L) CoreNative.glViewerDolly(handle, amount)
        }

        override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
            handle = CoreNative.glViewerCreate()
            loadPending()
        }

        override fun onSurfaceChanged(gl: GL10?, width: Int, height: Int) {
            if (handle != 0L) CoreNative.glViewerResize(handle, width, height)
        }

        override fun onDrawFrame(gl: GL10?) {
            if (handle != 0L) CoreNative.glViewerRender(handle)
        }

        private fun loadPending() {
            val path = pendingPath ?: modelPath ?: return
            if (handle == 0L) return
            val rc = CoreNative.glViewerLoad(handle, path)
            if (rc == 0) modelPath = path
            pendingPath = null
        }
    }
}
