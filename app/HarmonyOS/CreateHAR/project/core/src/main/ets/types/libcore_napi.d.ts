declare module 'libcore_napi.so' {
  interface CoreNapi {
    helloWorld(): number;
    helloWorldString(): string;
  }
  const coreNapi: CoreNapi;
  export default coreNapi;
}
