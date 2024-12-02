#include "gel/namespace.h"

#include "gel/to_string_helper.h"

namespace gel {
NamespaceList Namespace::namespaces_{};
void Namespace::Init() {
  InitClass();
}

auto Namespace::Get(const std::string& name) -> Namespace* {
  ASSERT(!name.empty());
  for (const auto& ns : namespaces_) {
    ASSERT(ns);
    const auto& ns_name = ns->GetName()->Get();
    if (ns_name == name)
      return ns;
  }
  return nullptr;
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
  return GetName()->Equals(rhs->AsNamespace()->GetName());
}

auto Namespace::HasPrefix(Symbol* rhs) const -> bool {
  ASSERT(rhs);
  const auto& s = rhs->Get();
  const auto& n = GetName()->Get();
  return s.starts_with(n + ":");
}

auto Namespace::Prefix(Symbol* rhs) const -> Symbol* {
  if (HasPrefix(rhs))
    return rhs;
  return Symbol::New(fmt::format("{0:s}:{1:s}", GetName()->Get(), rhs->Get()));
}

auto Namespace::ToString() const -> std::string {
  ToStringHelper<Namespace> helper;
  helper.AddField("name", GetName()->Get());
  helper.AddField("scope", GetScope());
  return helper;
}
}  // namespace gel