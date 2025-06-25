#include <cstdlib>
#include <glog/logging.h>

#include "gel/native_procedure.h"
#include "gel/plugin.h"

using namespace gel;

_DECLARE_NATIVE_PROCEDURE(say_hello, "test/say-hello");

NATIVE_PROCEDURE_F(say_hello) {
  DLOG(INFO) << "Hello World";
  return ReturnNull();
}

DEFINE_PLUGIN(Test) {
  InitNative<say_hello>();
  return EXIT_SUCCESS;
}