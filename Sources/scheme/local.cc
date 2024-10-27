#include "scheme/local.h"

#include "scheme/local_scope.h"
#include "scheme/type.h"

namespace scm {
auto LocalVariable::New(LocalScope* owner, const std::string& name, Type* value) -> LocalVariable* {
  ASSERT(owner);
  ASSERT(!name.empty());
  return New(owner, owner->GetNumberOfLocals(), name, value);
}

auto LocalVariable::New(LocalScope* owner, const Symbol* symbol, Type* value) -> LocalVariable* {
  ASSERT(owner);
  ASSERT(symbol);
  return New(owner, owner->GetNumberOfLocals(), symbol->Get(), value);
}
}  // namespace scm