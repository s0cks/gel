#include "gel/constructor.h"

#include <string>

#include "gel/common.h"
#include "gel/platform.h"
#include "gel/to_string_helper.h"
#include "gel/type.h"

namespace gel {
auto InitFn::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsInitFn())
    return false;
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return false;
}

auto InitFn::GetHashCode() const -> HashCode {
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return 0;
}

auto InitFn::Compare(Object* rhs) const -> bool {
  ASSERT(rhs);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return -1;
}

auto InitFn::ToString() const -> std::string {
  ToStringHelper<InitFn> helper{};
  return helper;
}

auto InitFn::CreateClass() -> Class* {
  ASSERT(kClass == nullptr);
  return Class::New(Fn::GetClass(), "InitFn");
}

auto InitFn::New(const ObjectList& args) -> InitFn* {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement?
  return nullptr;
}
}  // namespace gel