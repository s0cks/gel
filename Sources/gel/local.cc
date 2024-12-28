#include "gel/local.h"

#include "gel/local_scope.h"
#include "gel/platform.h"
#include "gel/pointer.h"
#include "gel/symbol.h"

namespace gel {
auto LocalVariable::Accept(PointerVisitor* vis) -> bool {
  ASSERT(vis);
  return vis->Visit(value_);
}

auto LocalVariable::Accept(PointerPointerVisitor* vis) -> bool {
  ASSERT(vis);
  return vis->Visit(&value_);
}

auto LocalVariable::GetValue() const -> Object* {
  return ptr() ? ((Object*)ptr()->GetObjectAddressPointer()) : nullptr;  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
}

void LocalVariable::SetValue(Object* rhs) {
  ASSERT(rhs);
  value_ = rhs->raw_ptr();
}

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
  return New(owner, owner->GetNumberOfLocals(), symbol->GetFullyQualifiedName(), value);
}
}  // namespace gel