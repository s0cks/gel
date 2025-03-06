#include "gel/local_scope.h"

#include <glog/logging.h>

#include "gel/common.h"
#include "gel/heap.h"
#include "gel/local.h"
#include "gel/object.h"
#include "gel/platform.h"
#include "gel/pointer.h"
#include "gel/to_string_helper.h"

namespace gel {
auto LocalScope::operator new(const size_t sz) -> void* {
  const auto heap = GetCurrentThreadHeap();
  ASSERT(heap);
  const auto address = heap->TryAllocate(sz);
  ASSERT(address != UNALLOCATED);
  return (void*)address;  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
}

void LocalScope::operator delete(void* ptr) {
  ASSERT(ptr);
  // do nothing
}

auto LocalScope::Union(const std::vector<LocalScope*>& scopes, LocalScope* parent) -> LocalScope* {
  if (scopes.empty())
    return New(parent);
  Array<LocalVariable*>* locals = Array<LocalVariable*>::New();
  std::ranges::for_each(std::begin(scopes), std::end(scopes), [&locals](LocalScope* scope) {
    for (auto idx = 0; idx < scope->GetNumberOfLocals(); idx++) {
      const auto local = scope->GetLocalAt(idx);
      ASSERT(local);
      locals->Add(local->GetValue());
    }
  });
  return new LocalScope(parent, locals);
}

auto LocalScope::Iterator::HasNext() const -> bool {
  return GetIndex() < GetScope()->GetNumberOfLocals();
}

auto LocalScope::Iterator::Next() -> LocalVariable* {
  const auto next = GetScope()->GetLocalAt(GetIndex());
  ASSERT(next);
  IncrementIndex();
  return next;
}

auto LocalScope::RecursiveIterator::HasNext() const -> bool {
  return GetIndex() < GetScope()->GetNumberOfLocals() || GetScope()->HasParent();
}

auto LocalScope::RecursiveIterator::Next() -> LocalVariable* {
  while ((GetScope()->IsEmpty() || GetIndex() >= GetScope()->GetNumberOfLocals()) && GetScope()->HasParent())
    SetScope(GetScope()->GetParent());
  ASSERT(GetIndex() <= GetScope()->GetNumberOfLocals());
  const auto next = GetScope()->GetLocalAt(GetIndex());
  IncrementIndex();
  return next;
}

auto LocalScope::VisitAllLocals(LocalVariableVisitor* vis, const bool recursive) -> bool {
  ASSERT(vis);
  LocalScope* current = this;
  do {
    LocalScope::Iterator iter(current);
    while (iter.HasNext()) {
      const auto next = iter.Next();
      ASSERT(next);
      if (!vis->VisitLocal(next))
        return false;
    }
    if (!recursive)
      break;
    current = current->GetParent();
  } while (current);
  return true;
}

auto LocalScope::HasLocal(const std::string& rhs) const -> bool {
  ASSERT(!rhs.empty());
  return HasLocal(Symbol::New(rhs));
}

auto LocalScope::Has(const std::string& symbol, const bool recursive) const -> bool {
  ASSERT(!symbol.empty());
  return Has(Symbol::New(symbol), recursive);
}

auto LocalScope::Has(Symbol* symbol, const bool recursive) const -> bool {
  ASSERT(symbol);
  LocalScope const* current = this;
  do {
    ASSERT(current);
    const auto local = current->FindIf(LocalVariable::HasSymbol(symbol));
    if (local)
      return true;
    if (!recursive)
      break;
    current = current->GetParent();
  } while (current);
  return false;
}

auto LocalScope::Lookup(Symbol* symbol, LocalVariable** result, const bool recursive) const -> bool {
  ASSERT(symbol);
  LocalScope const* current = this;
  do {
    ASSERT(current);
    {
      const auto local = current->FindIf(LocalVariable::HasSymbol(symbol));
      if (local) {
        (*result) = local;
        return true;
      }
    }
    {
      const auto local = current->FindIf(LocalVariable::HasSymbol(Symbol::CopyWithNewNamespace(symbol, "gel")));
      if (local) {
        (*result) = local;
        return true;
      }
    }
    if (!recursive)
      break;
    current = current->GetParent();
  } while (current);
  (*result) = nullptr;
  return false;
}

auto LocalScope::Lookup(const std::string& symbol, LocalVariable** result, const bool recursive) const -> bool {
  ASSERT(!symbol.empty());
  return Lookup(Symbol::New(symbol), result, recursive);
}

auto LocalScope::Add(Symbol* symbol, Object* value) -> LocalVariable* {
  ASSERT(symbol);
  return Add(LocalVariable::New(this, symbol, value));
}

auto LocalScope::Add(const std::string& symbol, Object* value) -> LocalVariable* {
  ASSERT(!symbol.empty());
  return Add(Symbol::New(symbol), value);
}

auto LocalScope::VisitPointers(PointerVisitor* vis) -> bool {
  ASSERT(vis);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto LocalScope::VisitPointerPointers(PointerPointerVisitor* vis) -> bool {
  ASSERT(vis);
  if (HasParent()) {
    auto parent = GetParent()->raw_ptr();
    if (!vis->Visit(&parent))
      return false;
    if (!GetParent()->raw_ptr()->Equals(parent)) {
      parent_ = parent->As<LocalScope>();
      ASSERT(parent_);
    }
  }

  if (locals_) {
    auto locals = GetLocals()->raw_ptr();
    if (!vis->Visit(&locals))
      return false;
    if (!GetLocals()->raw_ptr()->Equals(locals)) {
      locals_ = locals->As<Array<LocalVariable*>>();
      ASSERT(locals_);
    }
  }
  return true;
}

auto LocalScope::ToString() const -> std::string {
  ToStringHelper<LocalScope> helper;
  if (!IsEmpty())
    helper.AddField("locals", locals_);
  if (HasParent())
    helper.AddField("parent", (void*)GetParent());
  return helper;
}

#define __ (google::LogMessage(GetFile(), GetLine(), GetSeverity()).stream()) << GetIndentString()

auto LocalScopePrinter::VisitLocal(LocalVariable* local) -> bool {
  __ << "- " << (*local);
  return true;
}

auto LocalScopePrinter::PrintLocalScope(LocalScope* scope) -> bool {
  ASSERT(scope);
  __ << "Local Scope (" << scope->GetNumberOfLocals() << " locals):";
  Indent();
  do {
    if (!scope->VisitAllLocals(this)) {
      LOG(FATAL) << "failed to visit local scope: " << scope->ToString();
      return false;
    }

    if (!IsRecursive() || !scope->HasParent())
      break;
    scope = scope->GetParent();
  } while (true);
  Deindent();
  return true;
}

#undef __
}  // namespace gel