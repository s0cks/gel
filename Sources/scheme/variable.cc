#include "scheme/variable.h"

#include <sstream>

namespace scm {
auto Variable::ToString() const -> std::string {
  std::stringstream ss;
  ss << "Variable(";
  ss << "name=" << GetName();
  ss << ")";
  return ss.str();
}
}  // namespace scm