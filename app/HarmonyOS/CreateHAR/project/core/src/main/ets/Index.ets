import coreNapi from 'libcore_napi.so';

export function helloWorld(): number {
  return coreNapi.helloWorld();
}

export function helloWorldString(): string {
  return coreNapi.helloWorldString();
}
