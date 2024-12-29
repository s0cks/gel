#ifndef GEL_NATIVE_BINDINGS_H
#define GEL_NATIVE_BINDINGS_H

#include <filesystem>
#include <rpp/observables/dynamic_observable.hpp>

#include "gel/common.h"
#include "gel/rx.h"

namespace gel {
using PathPredicate = std::function<bool(std::filesystem::path)>;
class NativeBindings {
 public:
  static constexpr const auto kInitFuncName = "InitPlugin";
  using InitFunc = int (*)();

  static constexpr const auto kGetNameFuncName = "GetPluginName";
  using GetNameFunc = const char* (*)();

  static auto LoadFrom(const std::filesystem::path& path) -> int;
  static auto Load(const std::string& name) -> rx::dynamic_observable<int>;
  static auto CreateFilterFor(const std::string& name) -> PathPredicate;
};
}  // namespace gel

#endif  // GEL_NATIVE_BINDINGS_H
