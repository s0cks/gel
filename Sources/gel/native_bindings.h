#ifndef GEL_NATIVE_BINDINGS_H
#define GEL_NATIVE_BINDINGS_H

#include <filesystem>

#include "gel/common.h"

namespace gel {
class NativeBindings {
 public:
  static constexpr const auto kInitFuncName = "InitPlugin";
  using InitFunc = int (*)();

  static constexpr const auto kGetNameFuncName = "GetPluginName";
  using GetNameFunc = const char* (*)();

  static auto LoadFrom(const std::filesystem::path& path) -> int;
};
}  // namespace gel

#endif  // GEL_NATIVE_BINDINGS_H
