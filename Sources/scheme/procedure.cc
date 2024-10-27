#include "scheme/procedure.h"

#include <glog/logging.h>

#include "scheme/common.h"

namespace scm {
auto Macro::ToString() const -> std::string {
  std::stringstream ss;
  ss << "Macro(";
  ss << ")";
  return ss.str();
}
}  // namespace scm