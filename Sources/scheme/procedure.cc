#include "scheme/procedure.h"

#include <glog/logging.h>

#include "scheme/common.h"

namespace scm {
auto NativeProcedure::ToString() const -> std::string {
  std::stringstream ss;
  ss << "NativeProcedure(";
  ss << "symbol=" << GetSymbol();
  ss << ")";
  return ss.str();
}
}  // namespace scm