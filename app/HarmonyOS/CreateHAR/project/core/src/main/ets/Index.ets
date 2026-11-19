import coreNapi from 'libcore_napi.so';

export function helloWorld(): number {
  return coreNapi.helloWorld();
}

export function helloWorldString(): string {
  return coreNapi.helloWorldString();
}

/** Path consumed when XComponent surface is created. */
export function glSetModelPath(path: string): void {
  coreNapi.glSetModelPath(path);
}

/** Prefer on HarmonyOS: pass rawfile / asset bytes (avoids sandbox path fopen). */
export function glSetModelData(data: ArrayBuffer | Uint8Array): void {
  coreNapi.glSetModelData(data);
}

/** MakeCurrent + render + eglSwapBuffers for all GL XComponents. */
export function glPresentAll(): void {
  coreNapi.glPresentAll();
}

/** Orbit all GL surfaces (ArkTS onTouch → Arcball). */
export function glOrbitAll(dx: number, dy: number): void {
  coreNapi.glOrbitAll(dx, dy);
}

export function glOrbitEndAll(): void {
  coreNapi.glOrbitEndAll();
}

/** Dolly all GL surfaces: amount > 0 moves camera forward. */
export function glDollyAll(amount: number): void {
  coreNapi.glDollyAll(amount);
}

/** Move point light on X for all surfaces: amount > 0 → right. */
export function glLightMoveXAll(amount: number): void {
  coreNapi.glLightMoveXAll(amount);
}

/** Drag point light XY for all surfaces (Pro toggle). */
export function glLightDragAll(dx: number, dy: number): void {
  coreNapi.glLightDragAll(dx, dy);
}

export function glExplodeAll(factor: number = 1): void {
  coreNapi.glExplodeAll(factor);
}

export function glImplodeAll(): void {
  coreNapi.glImplodeAll();
}

/** Screen pick (surface pixels); returns mesh id or -1. */
export function glPickAll(sx: number, sy: number): number {
  return coreNapi.glPickAll(sx, sy);
}

export function glClearSelectionAll(): void {
  coreNapi.glClearSelectionAll();
}

export function glViewerCreate(): bigint {
  return coreNapi.glViewerCreate();
}

export function glViewerDestroy(handle: bigint): void {
  coreNapi.glViewerDestroy(handle);
}

export function glViewerLoad(handle: bigint, path: string): number {
  return coreNapi.glViewerLoad(handle, path);
}

export function glViewerResize(handle: bigint, width: number, height: number): void {
  coreNapi.glViewerResize(handle, width, height);
}

export function glViewerRender(handle: bigint): void {
  coreNapi.glViewerRender(handle);
}

export function glViewerOrbit(handle: bigint, dx: number, dy: number): void {
  coreNapi.glViewerOrbit(handle, dx, dy);
}

export function glViewerOrbitEnd(handle: bigint): void {
  coreNapi.glViewerOrbitEnd(handle);
}

/** Defaults: sensitivity=1, brakeFactor=0.10 (↑ brake = stops faster; drag unaffected). */
export function glViewerSetOrbitFeel(handle: bigint, sensitivity: number,
                                     brakeFactor: number): void {
  coreNapi.glViewerSetOrbitFeel(handle, sensitivity, brakeFactor);
}

export function glViewerZoom(handle: bigint, scale: number): void {
  coreNapi.glViewerZoom(handle, scale);
}

export function glViewerDolly(handle: bigint, amount: number): void {
  coreNapi.glViewerDolly(handle, amount);
}

export function glViewerLightMoveX(handle: bigint, amount: number): void {
  coreNapi.glViewerLightMoveX(handle, amount);
}
