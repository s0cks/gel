#include "scheme/procedure.h"

#include <glog/logging.h>

#include "scheme/common.h"
#include "scheme/runtime.h"

namespace scm {
auto Procedure::CreateClass() -> Class* {
  return Class::New(Object::GetClass(), "Procedure");
}
}  // namespace scm