#include <gel/plugin.h>
#include <glog/logging.h>

#include <cstdlib>

#include "gel/native_procedure.h"

using namespace gel;

_DECLARE_NATIVE_PROCEDURE(env_get, "env/get");

NATIVE_PROCEDURE_F(env_get) {
  NativeArgument<0, String> key(args);
  if (!key)
    return Throw(key);
  const auto value = getenv(key->Get().c_str());
  if (value)
    return ReturnNew<String>(std::string(value));
  return ReturnNull();
}

DEFINE_PLUGIN(env) {
  env_get::Init();
  return EXIT_SUCCESS;
}