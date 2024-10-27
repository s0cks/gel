#include "scheme/local_scope.h"

#include <glog/logging.h>

#include "scheme/local.h"
#include "scheme/type.h"

namespace scm {
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
  if (Has(local->GetName()))
    return false;
  locals_.push_back(local);
  if (!local->HasOwner())
    local->SetOwner(this);
  return true;
}

auto LocalScope::Add(Symbol* symbol, Type* value) -> bool {
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
    if (local->HasValue()) {
      DLOG(INFO) << "added " << local->GetName() << " := " << local->GetValue()->ToString() << " to scope.";
    } else {
      DLOG(INFO) << "added " << local->GetName() << " to scope.";
    }
  }
  DLOG(INFO) << "added " << scope->GetNumberOfLocals() << " locals to scope.";
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

auto LocalScope::VisitAllLocals(LocalVariableVisitor* vis) -> bool {
  ASSERT(vis);
  for (const auto& local : locals_) {
    if (!vis->VisitLocal(local))
      return false;
  }
  return true;
}
}  // namespace scm