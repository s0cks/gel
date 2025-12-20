#include "constructor.h"

#include <string>

#include "common.h"
#include "platform.h"
#include "to_string_helper.h"
#include "type.h"

namespace gel {
auto Constructor::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsConstructor())
    return false;
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return false;
}

auto Constructor::GetHashCode() const -> HashCode {
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return 0;
}

auto Constructor::Compare(Object* rhs) const -> bool {
  ASSERT(rhs);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return -1;
}

auto Constructor::ToString() const -> std::string {
  ToStringHelper<Constructor> helper{};
  return helper;
}

auto Constructor::CreateClass() -> Class* {
  ASSERT(kClass == nullptr);
  return Class::New(Fn::GetClass(), "Constructor");
}

auto Constructor::New(const ObjectList& args) -> Constructor* {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement?
  return nullptr;
}
}  // namespace gel
