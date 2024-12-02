#include "gel/module.h"

#include "gel/common.h"
#include "gel/to_string_helper.h"

namespace gel {
auto Module::ToString() const -> std::string {
  ToStringHelper<Module> helper;
  helper.AddField("name", GetName());
  return helper;
}

auto Module::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsModule())
    return false;
  const auto other = rhs->AsModule();
  ASSERT(other);
  return GetName()->Equals(other->GetName());
}

auto Module::CreateClass() -> Class* {
  ASSERT(kClass == nullptr);
  return Class::New(Object::GetClass(), "Module");
}

auto Module::New(const ObjectList& args) -> Module* {
  NOT_IMPLEMENTED(FATAL);
}
}  // namespace gel