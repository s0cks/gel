#include <cstdlib>
#include <glog/logging.h>

#include "native_procedure.h"
#include "plugin.h"

using namespace gel;

_DECLARE_NATIVE_PROCEDURE(say_hello, "os/say-hello");

NATIVE_PROCEDURE_F(say_hello) {
  DLOG(INFO) << "Hello World";
  return ReturnNull();
}

DEFINE_PLUGIN(rx) {
  InitNative<say_hello>();
  return EXIT_SUCCESS;
}
