#include "gel/symbol.h"

#include "gel/namespace.h"
#include "gel/to_string_helper.h"

namespace gel {
void Symbol::SetNamespace(Namespace* ns) {
  ASSERT(ns);
  return SetNamespace(ns->GetSymbol()->GetSymbolName());
}

auto Symbol::Equals(const std::string& rhs) const -> bool {
  ASSERT(!rhs.empty());
  return GetFullyQualifiedName() == rhs;
}

auto Symbol::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsSymbol())
    return false;
  const auto other = rhs->AsSymbol();
  ASSERT(other);
  if ((HasNamespace() && !other->HasNamespace()) || (!HasNamespace() && other->HasNamespace()) ||
      (GetNamespace() != other->GetNamespace()))
    return false;
  return GetSymbolName() == other->GetSymbolName();
}

auto Symbol::HashCode() const -> uword {
  uword hash = 0;
  if (HasNamespace())
    CombineHash(hash, GetNamespace());
  CombineHash(hash, GetSymbolName());
  return hash;
}

auto Symbol::CreateClass() -> Class* {
  return Class::New(Object::GetClass(), kClassName);
}

auto Symbol::New(const ObjectList& args) -> Symbol* {
  NOT_IMPLEMENTED(FATAL);
}

auto Symbol::ToString() const -> std::string {
  ToStringHelper<Symbol> helper;
  helper.AddField("value", GetFullyQualifiedName());
  return helper;
}

auto Symbol::New(const std::string& ns, const std::string& type, const std::string& name) -> Symbol* {
  ASSERT(!name.empty());
  return new Symbol(ns, type, name);
}
}  // namespace gel