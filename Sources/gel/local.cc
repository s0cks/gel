#include "gel/local.h"

#include <cstddef>
#include <functional>
#include <string>

#include "gel/common.h"
#include "gel/heap.h"
#include "gel/local_scope.h"
#include "gel/platform.h"
#include "gel/symbol.h"
#include "gel/to_string_helper.h"

namespace gel {
auto LocalVariable::operator new(const size_t sz) -> void* {
  const auto heap = GetCurrentThreadHeap();
  ASSERT(heap);
  const auto address = heap->TryAllocate(sz);
  ASSERT(address != UNALLOCATED);
  return (void*)address;  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
}

void LocalVariable::operator delete(void* ptr) {
  ASSERT(ptr);
  // do nothing
}

auto LocalVariable::HasSymbol(Symbol* rhs) -> std::function<bool(LocalVariable*)> {
  ASSERT(rhs);
  return [rhs](LocalVariable* local) {
    return local && local->GetSymbol()->Equals(rhs);
  };
}

auto LocalVariable::HasSymbol(const std::string& symbol) -> std::function<bool(LocalVariable*)> {
  ASSERT(!symbol.empty());
  return [&symbol](LocalVariable* local) {
    return local && local->GetSymbol()->Equals(symbol);
  };
}

void LocalVariable::SetValue(Object* rhs) {
  ASSERT(rhs);
  value_ = rhs;
}

auto LocalVariable::GetValue() const -> Object* {
  return value_;
}

auto LocalVariable::IsGlobal() const -> bool {
  return HasOwner() && GetOwner()->IsRoot();
}

auto LocalVariable::ToString() const -> std::string {
  ToStringHelper<LocalVariable> helper{};
  helper.AddField("owner", GetOwner());
  helper.AddField("index", GetIndex());
  helper.AddField("symbol", GetSymbol());
  if (HasValue())
    helper.AddField("value", GetValue());
  return helper;
}

auto LocalVariable::VisitPointers(PointerVisitor* vis) -> bool {
  ASSERT(vis);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return true;
}

auto LocalVariable::VisitPointerPointers(PointerPointerVisitor* vis) -> bool {
  ASSERT(vis);
  if (!VisitPointerPointer(vis, &owner_))
    return false;
  if (!VisitPointerPointer(vis, &symbol_))
    return false;
  if (!VisitPointerPointer(vis, &value_))
    return false;
  return true;
}

auto LocalVariable::New(LocalScope* owner, String* name, Object* value) -> LocalVariable* {
  ASSERT(owner);
  ASSERT(name);
  return New(owner, owner->GetNumberOfLocals(), Symbol::New(name), value);
}

auto LocalVariable::New(LocalScope* owner, Symbol* symbol, Object* value) -> LocalVariable* {
  ASSERT(owner);
  ASSERT(symbol);
  return New(owner, owner->GetNumberOfLocals(), symbol, value);
}

auto LocalVariable::New(LocalScope* owner, const std::string& name, Object* value) -> LocalVariable* {
  ASSERT(owner);
  ASSERT(!name.empty());
  return New(owner, String::New(name), value);
}
}  // namespace gel