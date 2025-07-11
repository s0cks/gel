#include "gel/procedure.h"

#include <glog/logging.h>

#include "gel/common.h"
#include "gel/runtime.h"

namespace gel {
auto Fn::CreateClass() -> Class* {
  return Class::New(Object::GetClass(), "fn");
}
}  // namespace gel