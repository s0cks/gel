#include "gel/local_scope.h"

#include <glog/logging.h>

#include "gel/common.h"
#include "gel/local.h"
#include "gel/object.h"
#include "gel/pointer.h"
#include "gel/to_string_helper.h"

namespace gel {
auto LocalScope::Iterator::HasNext() const -> bool {
  return GetIndex() < GetScope()->GetNumberOfLocals();
}

auto LocalScope::Iterator::Next() -> LocalVariable* {
  const auto next = GetScope()->GetLocalAt(GetIndex());
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

auto LocalScope::Has(const std::string& name, const bool recursive) -> bool {
  for (const auto& local : locals_) {
    if (local->GetName() == name)
      return true;
  }
  return recursive && HasParent() ? GetParent()->Has(name, recursive) : false;
}

auto LocalScope::Has(const Symbol* symbol, const bool recursive) -> bool {
  ASSERT(symbol);
  return Has(symbol->Get(), recursive);
}

auto LocalScope::Add(LocalVariable* local) -> bool {
  ASSERT(local);
  if (Has(local->GetName())) {
    DLOG(ERROR) << "cannot add duplicate local: " << (*local);
    return false;
  }
  locals_.push_back(local);
  if (!local->HasOwner())
    local->SetOwner(this);
  return true;
}

auto LocalScope::Add(Symbol* symbol, Object* value) -> bool {
  ASSERT(symbol);
  return Add(symbol->Get(), value);
}

auto LocalScope::Add(LocalScope* scope) -> bool {
  ASSERT(scope);
  for (const auto& local : scope->locals_) {
    if (!Add(local->GetName(), local->GetValue())) {
      LOG(ERROR) << "failed to add local " << local->GetName() << " to scope.";
      return false;
    }
  }
  return true;
}

auto LocalScope::Lookup(const std::string& name, LocalVariable** result, const bool recursive) -> bool {
  ASSERT(!name.empty());
  for (const auto& local : locals_) {
    if (local->GetName() == name) {
      (*result) = local;
      return true;
    }
  }
  return recursive && HasParent() ? GetParent()->Lookup(name, result, recursive) : false;
}

auto LocalScope::Lookup(const Symbol* symbol, LocalVariable** result, const bool recursive) -> bool {
  ASSERT(symbol);
  return Lookup(symbol->Get(), result, recursive);
}

static inline auto operator<<(std::ostream& stream, const std::vector<LocalVariable*>& rhs) -> std::ostream& {
  stream << "[";
  auto remaining = rhs.size();
  for (const auto& local : rhs) {
    stream << (*local);
    if (--remaining >= 1)
      stream << ", ";
  }
  stream << "]";
  return stream;
}

auto LocalScope::ToString() const -> std::string {
  ToStringHelper<LocalScope> helper;
  helper.AddField("locals", locals_);
  helper.AddField("parent", GetParent());
  return helper;
}

auto LocalScope::VisitAllLocals(LocalVariableVisitor* vis) -> bool {
  ASSERT(vis);
  for (const auto& local : locals_) {
    if (!vis->VisitLocal(local))
      return false;
  }
  return true;
}

auto LocalScope::Accept(PointerVisitor* vis) -> bool {
  ASSERT(vis);
  auto scope = this;
  while (scope) {
    for (const auto& local : scope->locals_) {
      if (!local->Accept(vis))
        return false;
    }
    scope = scope->GetParent();
  }
  return true;
}

auto LocalScope::VisitLocalPointers(const std::function<bool(Pointer**)>& vis, const bool recursive) -> bool {
  auto scope = this;
  do {
    for (const auto& local : scope->locals_) {
      ASSERT(local);
      if (!local->Accept(vis))
        return false;
    }
    scope = scope->GetParent();
  } while (scope && recursive);
  return true;
}

auto LocalScope::VisitLocals(const std::function<bool(Pointer*)>& vis, const bool recursive) -> bool {
  auto scope = this;
  do {
    for (const auto& local : scope->locals_) {
      ASSERT(local);
      if (!vis(local->ptr()))
        return false;
    }
    scope = scope->GetParent();
  } while (scope && recursive);
  return true;
}

auto LocalScope::Accept(PointerPointerVisitor* vis) -> bool {
  ASSERT(vis);
  auto scope = this;
  while (scope) {
    for (const auto& local : scope->locals_) {
      if (!local->Accept(vis))
        return false;
    }
    scope = scope->GetParent();
  }
  return true;
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