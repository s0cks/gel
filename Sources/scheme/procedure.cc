#include "scheme/procedure.h"

#include <glog/logging.h>

#include "scheme/common.h"
#include "scheme/runtime.h"

namespace scm {
Class* Procedure::kClass = nullptr;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
void Procedure::Init() {
  ASSERT(kClass == nullptr);
  kClass = Class::New(Object::GetClass(), "Procedure");
  ASSERT(kClass);
}
}  // namespace scm