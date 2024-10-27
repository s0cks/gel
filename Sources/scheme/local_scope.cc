#include "scheme/local_scope.h"

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
}  // namespace scm