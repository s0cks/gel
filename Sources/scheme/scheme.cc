#include "scheme/scheme.h"

#include <sstream>

namespace scm {
auto GetVersion() -> std::string {
  std::stringstream ss;
  ss << SCM_VERSION_MAJOR << ".";
  ss << SCM_VERSION_MINOR << ".";
  ss << SCM_VERSION_PATCH;
  return ss.str();
}
}  // namespace scm