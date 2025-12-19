#include <__ostream/basic_ostream.h>
#include <sstream>
#include <string>

#include "gel.h"

namespace gel {
auto GetVersion() -> std::string {
  std::stringstream ss;
  ss << GEL_VERSION_MAJOR << ".";
  ss << GEL_VERSION_MINOR << ".";
  ss << GEL_VERSION_PATCH;
  return ss.str();
}
}  // namespace gel
