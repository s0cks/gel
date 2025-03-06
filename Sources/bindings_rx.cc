#include <gel/plugin.h>
#include <glog/logging.h>

#include <cstdlib>

#include "gel/native_procedure.h"

using namespace gel;

_DECLARE_NATIVE_PROCEDURE(say_hello, "rx/say-hello");

NATIVE_PROCEDURE_F(say_hello) {
  DLOG(INFO) << "Hello World";
  return ReturnNull();
}

DEFINE_PLUGIN(rx) {
  InitNative<say_hello>();
  return EXIT_SUCCESS;
}