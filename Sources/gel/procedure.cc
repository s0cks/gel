#include "gel/procedure.h"

#include <glog/logging.h>

#include "gel/common.h"
#include "gel/runtime.h"

namespace gel {
auto Procedure::CreateClass() -> Class* {
  return Class::New(Object::GetClass(), "Procedure");
}
}  // namespace gel