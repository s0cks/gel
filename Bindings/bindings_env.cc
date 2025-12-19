#include <cstdlib>
#include <glog/logging.h>
#include <string>

#include "native_procedure.h"
#include "object.h"
#include "plugin.h"

using namespace gel;

_DECLARE_NATIVE_PROCEDURE(env_get, "env/get");

NATIVE_PROCEDURE_F(env_get) {
  NativeArgument<0, String> key(args);
  CHECK_NATIVE_ARG(key);
  const auto value = getenv(key->Get().c_str());
  if (value)
    return ReturnNew<String>(std::string(value));
  return ReturnNull();
}

DEFINE_PLUGIN(env) {
  env_get::Init();
  return EXIT_SUCCESS;
}
