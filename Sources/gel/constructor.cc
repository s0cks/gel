#include "gel/constructor.h"

#include "gel/common.h"
#include "gel/to_string_helper.h"

namespace gel {
auto Constructor::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsConstructor())
    return false;
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return false;
}

auto Constructor::HashCode() const -> uword {
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return 0;
}

auto Constructor::ToString() const -> std::string {
  ToStringHelper<Constructor> helper{};
  return helper;
}

auto Constructor::CreateClass() -> Class* {
  ASSERT(kClass == nullptr);
  return Class::New(Procedure::GetClass(), "Constructor");
}

auto Constructor::New(const ObjectList& args) -> Constructor* {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement?
  return nullptr;
}
}  // namespace gel