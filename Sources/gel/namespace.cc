#include "gel/namespace.h"

#include <algorithm>

#include "gel/common.h"
#include "gel/local.h"
#include "gel/to_string_helper.h"

namespace gel {
NamespaceList Namespace::namespaces_{};

auto Namespace::IsKernelNamespace() const -> bool {
  return GetName() == "_kernel";
}

auto Namespace::HashCode() const -> uword {
  uword hash = 0;
  CombineHash(hash, GetSymbol()->HashCode());
  return hash;
}

auto Namespace::FindNamespace(const std::string& name) -> Namespace* {
  const auto pos = std::ranges::find_if(namespaces_, [&name](Namespace* ns) {
    ASSERT(ns);
    return ns->GetSymbol()->Equals(name);
  });
  return pos != std::end(namespaces_) ? (*pos) : nullptr;
}

auto Namespace::FindNamespace(Symbol* rhs) -> Namespace* {
  ASSERT(rhs);
  return FindNamespace(rhs->GetSymbolName());
}

auto Namespace::HasSymbol(const std::string& rhs) const -> bool {
  ASSERT(!rhs.empty());
  LocalVariable* local = nullptr;
  return GetScope()->Lookup(rhs, &local, false);
}

auto Namespace::Get(const std::string& rhs) const -> Object* {
  ASSERT(!rhs.empty());
  LocalVariable* local = nullptr;
  if (!GetScope()->Lookup(rhs, &local, false))
    return nullptr;
  ASSERT(local);
  return local->GetValue();
}

auto Namespace::Get(Symbol* rhs) const -> Object* {
  ASSERT(rhs);
  LocalVariable* local = nullptr;
  if (!GetScope()->Lookup(rhs, &local, false))
    return nullptr;
  ASSERT(local);
  return local->GetValue();
}

auto Namespace::HasSymbol(Symbol* rhs) const -> bool {
  ASSERT(rhs);
  LocalVariable* local = nullptr;
  return GetScope()->Lookup(rhs, &local, false);
}

auto Namespace::GetName() const -> const std::string& {
  return GetSymbol()->GetSymbolName();
}

auto Namespace::CreateSymbol(const std::string& rhs) -> Symbol* {
  ASSERT(!rhs.empty());
  if (IsKernelNamespace())
    return Symbol::New(rhs);
  if (Contains(rhs, '/')) {
    const auto last = rhs.find_last_of('/');
    ASSERT(last != std::string::npos);
    return Symbol::New(GetSymbol()->GetSymbolName(), rhs.substr(last + 1));
  }
  return Symbol::New(GetSymbol()->GetSymbolName(), rhs);
}

auto Namespace::CreateClass() -> Class* {
  ASSERT(kClass == nullptr);
  return Class::New(Object::GetClass(), "Namespace");
}

auto Namespace::New(const ObjectList& args) -> Namespace* {
  LOG(FATAL) << "cannot create a new Namespace";
}

auto Namespace::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsNamespace())
    return false;
  const auto other = rhs->AsNamespace();
  ASSERT(other);
  return GetSymbol()->Equals(other->GetSymbol());
}

auto Namespace::ToString() const -> std::string {
  ToStringHelper<Namespace> helper;
  helper.AddField("symbol", GetSymbol()->GetFullyQualifiedName());
  helper.AddField("scope", GetScope());
  return helper;
}
}  // namespace gel