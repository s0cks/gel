#include "scheme/error.h"

#include <sstream>

namespace scm {
auto Error::Equals(Type* rhs) const -> bool {
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