#include "scheme/local.h"

#include "scheme/local_scope.h"

namespace scm {
auto LocalVariable::IsGlobal() const -> bool {
  return HasOwner() && GetOwner()->IsRoot();
}

auto LocalVariable::New(LocalScope* owner, const std::string& name, Object* value) -> LocalVariable* {
  ASSERT(owner);
  ASSERT(!name.empty());
  return New(owner, owner->GetNumberOfLocals(), name, value);
}

auto LocalVariable::New(LocalScope* owner, const Symbol* symbol, Object* value) -> LocalVariable* {
  ASSERT(owner);
  ASSERT(symbol);
  return New(owner, owner->GetNumberOfLocals(), symbol->Get(), value);
}
}  // namespace scm