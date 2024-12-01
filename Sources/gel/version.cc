#include "gel/gel.h"

#include <sstream>

namespace gel {
auto GetVersion() -> std::string {
  std::stringstream ss;
  ss << GEL_VERSION_MAJOR << ".";
  ss << GEL_VERSION_MINOR << ".";
  ss << GEL_VERSION_PATCH;
  return ss.str();
}
}  // namespace gel