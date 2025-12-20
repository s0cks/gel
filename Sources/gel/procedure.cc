#include "procedure.h"

#include <glog/logging.h>

#include "common.h"
#include "runtime.h"

namespace gel {
auto Fn::CreateClass() -> Class* {
  return Class::New(Object::GetClass(), "Fn");
}
}  // namespace gel
