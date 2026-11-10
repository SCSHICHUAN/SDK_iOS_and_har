#include "core/core.h"

#include <cstdio>

int core_hello_world(void) {
  std::printf("Hello World!\n");
  return 0;
}

const char* core_hello_world_string(void) {
  return "Hello World!";
}

std::string core_hello_world_string_cpp() {
  return std::string(core_hello_world_string());
}
