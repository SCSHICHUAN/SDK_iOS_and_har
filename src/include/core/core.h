#ifndef CORE_CORE_H_
#define CORE_CORE_H_

#ifdef __cplusplus
#include <string>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Print "Hello World!" to stdout.
 * Returns 0 on success.
 */
int core_hello_world(void);

/**
 * C / ABI-stable: return a static "Hello World!" string (do not free).
 * Usable from C, JNI, NAPI, Swift, etc.
 */
const char* core_hello_world_string(void);

#ifdef __cplusplus
}  /* extern "C" */

/**
 * C++ API: return "Hello World!" as std::string.
 */
std::string core_hello_world_string_cpp();
#endif

#endif  /* CORE_CORE_H_ */
