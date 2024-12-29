#ifndef GEL_PLUGIN_H
#define GEL_PLUGIN_H

#include "gel/common.h"

namespace gel {
#define DEFINE_PLUGIN(Name)                        \
  extern "C" auto GetPluginName() -> const char* { \
    return #Name;                                  \
  }                                                \
  extern "C" auto InitPlugin() -> int

}  // namespace gel

#endif  // GEL_PLUGIN_H
