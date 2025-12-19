#include "procedure.h"

#include <glog/logging.h>

#include "common.h"
#include "runtime.h"

namespace gel {
auto Procedure::CreateClass() -> Class* {
  return Class::New(Object::GetClass(), "Procedure");
}
}  // namespace gel
