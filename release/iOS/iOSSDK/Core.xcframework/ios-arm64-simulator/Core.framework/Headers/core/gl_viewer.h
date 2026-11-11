#ifndef CORE_GL_VIEWER_H_
#define CORE_GL_VIEWER_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * C boundary for the GLES model browser.
 *
 * Internals (AR-aligned):
 *   RendererData  ≈ SCRendererData   (frame render)
 *   Model/Mesh    ≈ model.h/mesh.h   (asset + Draw submit)
 *   Camera/Shader ≈ Camera.h/shader.h
 * Platform surface (FBO/RBO/present) stays in the Pro app ≈ SCRenderer.
 *
 * Caller must make a GLES3 context current before create/load/render.
 */
typedef struct CoreGlViewer CoreGlViewer;

CoreGlViewer* core_gl_viewer_create(void);
void core_gl_viewer_destroy(CoreGlViewer* viewer);

/** Load a model via Assimp (obj/fbx/glb/...). Returns 0 on success. */
int core_gl_viewer_load(CoreGlViewer* viewer, const char* path);

/** Load from memory bytes (hint e.g. ".obj"). Returns 0 on success. */
int core_gl_viewer_load_memory(CoreGlViewer* viewer, const void* data, int size,
                               const char* hint);

void core_gl_viewer_resize(CoreGlViewer* viewer, int width, int height);
void core_gl_viewer_render(CoreGlViewer* viewer);

/** Orbit from touch drag. */
void core_gl_viewer_orbit(CoreGlViewer* viewer, float dx, float dy);
/** Pinch zoom: scale > 1 zooms in. */
void core_gl_viewer_zoom(CoreGlViewer* viewer, float scale);
/** Dolly: amount > 0 moves camera forward (closer to model). */
void core_gl_viewer_dolly(CoreGlViewer* viewer, float amount);
/** Move point light on X: amount > 0 → right. */
void core_gl_viewer_light_move_x(CoreGlViewer* viewer, float amount);
/** Drag point light in XY (Pro touch deltas). */
void core_gl_viewer_light_drag(CoreGlViewer* viewer, float dx, float dy);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CORE_GL_VIEWER_H_ */
