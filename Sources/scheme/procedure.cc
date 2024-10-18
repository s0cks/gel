#include "scheme/procedure.h"

#include <glog/logging.h>

#include "scheme/common.h"

namespace scm {
auto Lambda::Equals(Type* rhs) const -> bool {
  if (!rhs->IsLambda())
    return false;
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return false;
}

auto Lambda::Apply(scm::Runtime*, scm::Datum*) const -> Datum* {
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return Null::Get();
}

auto Lambda::ToString() const -> std::string {
  std::stringstream ss;
  ss << "Lambda(";
  ss << ")";
  return ss.str();
}

auto Macro::ToString() const -> std::string {
  std::stringstream ss;
  ss << "Macro(";
  ss << ")";
  return ss.str();
}
}  // namespace scm