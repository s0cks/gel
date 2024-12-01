#include "gel/variable.h"

#include <sstream>

namespace gel {
auto Variable::ToString() const -> std::string {
  std::stringstream ss;
  ss << "Variable(";
  ss << "name=" << GetName();
  ss << ")";
  return ss.str();
}
}  // namespace gel