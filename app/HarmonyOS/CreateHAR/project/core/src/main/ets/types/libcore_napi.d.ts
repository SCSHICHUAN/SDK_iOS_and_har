declare module 'libcore_napi.so' {
  interface CoreNapi {
    helloWorld(): number;
    helloWorldString(): string;
    glSetModelPath(path: string): void;
    glSetModelData(data: ArrayBuffer | Uint8Array): void;
    glPresentAll(): void;
    glOrbitAll(dx: number, dy: number): void;
    glOrbitEndAll(): void;
    glDollyAll(amount: number): void;
    glLightMoveXAll(amount: number): void;
    glLightDragAll(dx: number, dy: number): void;
    glExplodeAll(factor: number): void;
    glImplodeAll(): void;
    /** Screen pick in surface pixels; returns mesh id or -1. */
    glPickAll(sx: number, sy: number): number;
    glClearSelectionAll(): void;
    glViewerCreate(): bigint;
    glViewerDestroy(handle: bigint): void;
    glViewerLoad(handle: bigint, path: string): number;
    glViewerResize(handle: bigint, width: number, height: number): void;
    glViewerRender(handle: bigint): void;
    glViewerOrbit(handle: bigint, dx: number, dy: number): void;
    glViewerOrbitEnd(handle: bigint): void;
    /** Defaults: sensitivity=1, brakeFactor=0.10 (drag ratio ⊥ release brake) */
    glViewerSetOrbitFeel(handle: bigint, sensitivity: number, brakeFactor: number): void;
    glViewerZoom(handle: bigint, scale: number): void;
    glViewerDolly(handle: bigint, amount: number): void;
    glViewerLightMoveX(handle: bigint, amount: number): void;
  }
  const coreNapi: CoreNapi;
  export default coreNapi;
}
