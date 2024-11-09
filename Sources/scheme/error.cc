#include "scheme/error.h"

#include <sstream>

#include "scheme/common.h"
#include "scheme/object.h"

namespace scm {
Class* Error::kClass = nullptr;
void Error::Init() {
  ASSERT(kClass == nullptr);
  kClass = Class::New(Object::GetClass(), "Error");
  ASSERT(kClass);
}

auto Error::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsError())
    return false;
  ASSERT(rhs->IsError());
  const auto other = rhs->AsError();
  return GetMessage() == other->GetMessage();
}

auto Error::ToString() const -> std::string {
  std::stringstream ss;
  ss << "Error(";
  ss << "message=" << GetMessage();
  ss << ")";
  return ss.str();
}
}  // namespace scm