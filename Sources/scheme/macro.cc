#include "scheme/macro.h"

#include <sstream>

namespace scm {
auto Macro::ToString() const -> std::string {
  std::stringstream ss;
  ss << "Macro(";
  ss << "symbol=" << GetSymbol();
  ss << ")";
  return ss.str();
}
}  // namespace scm