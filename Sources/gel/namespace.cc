#include "gel/namespace.h"

#include "gel/to_string_helper.h"

namespace gel {
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

auto Namespace::ToString() const -> std::string {
  ToStringHelper<Namespace> helper;
  helper.AddField("name", GetName()->Get());
  return helper;
}
}  // namespace gel