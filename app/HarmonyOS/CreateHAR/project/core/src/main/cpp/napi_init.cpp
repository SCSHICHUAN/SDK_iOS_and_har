#include "napi/native_api.h"

#include "core/core.h"

static napi_value HelloWorld(napi_env env, napi_callback_info /*info*/) {
  const int ret = core_hello_world();
  napi_value result;
  napi_create_int32(env, ret, &result);
  return result;
}

static napi_value HelloWorldString(napi_env env, napi_callback_info /*info*/) {
  const char* text = core_hello_world_string();
  napi_value result;
  napi_create_string_utf8(env, text != nullptr ? text : "", NAPI_AUTO_LENGTH, &result);
  return result;
}

EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports) {
  napi_property_descriptor desc[] = {
      {"helloWorld", nullptr, HelloWorld, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"helloWorldString", nullptr, HelloWorldString, nullptr, nullptr, nullptr, napi_default,
       nullptr}};
  napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
  return exports;
}
EXTERN_C_END

static napi_module g_module = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "core_napi",
    .nm_priv = nullptr,
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterCoreNapiModule(void) {
  napi_module_register(&g_module);
}
