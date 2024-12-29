#include "gel/native_bindings.h"

#include <fmt/format.h>
#include <glog/logging.h>

#include <filesystem>
#include <rpp/observables/dynamic_observable.hpp>

#include "gel/common.h"
#include "gel/runtime.h"
#include "gel/shared_lib.h"

namespace gel {
class NativeBinding {
  DEFINE_NON_COPYABLE_TYPE(NativeBinding);

 private:
  SharedLibrary lib_;
  NativeBindings::GetNameFunc get_name_;
  NativeBindings::InitFunc init_;

 public:
  explicit NativeBinding(const std::string& filename) :
    lib_(filename),
    get_name_(lib_.DlSym<NativeBindings::GetNameFunc>(NativeBindings::kGetNameFuncName)),
    init_(lib_.DlSym<NativeBindings::InitFunc>(NativeBindings::kInitFuncName)) {
    ASSERT(get_name_);
    ASSERT(init_);
  }
  ~NativeBinding() = default;

  auto GetName() const -> std::string {
    ASSERT(get_name_);
    return get_name_();
  }

  auto Init() const -> int {
    ASSERT(init_);
    return init_();
  }
};

auto NativeBindings::CreateFilterFor(const std::string& name) -> PathPredicate {
#if defined(OS_IS_OSX)
  const auto target_filename = fmt::format("lib{}.dylib", name);
#elif defined(OS_IS_LINUX)
  const auto target_filename = fmt::format("lib{}.so", name);
#elif defined(OS_IS_WINDOWS)
  const auto target_filename = fmt::format("{}.dll", name);
#else
#error "Unsupported Operating System"
#endif
  return [target_filename](std::filesystem::path p) -> bool {
    return std::filesystem::is_regular_file(p) && p.filename() == target_filename;
  };
}

auto NativeBindings::LoadFrom(const std::filesystem::path& p) -> int {
  NativeBinding binding(p);
  const auto name = binding.GetName();
  DVLOG(100) << "initializing " << name << " plugin....";
  const auto status = binding.Init();  // TODO: better lib open error detection
  VLOG_IF(1000, status != EXIT_SUCCESS) << "failed to initialize the " << name
                                        << " plugin, plugin returned with status: " << status;
  return status;
}

auto NativeBindings::Load(const std::string& filename) -> rx::dynamic_observable<int> {
  return LsGelPath() | rx::operators::filter(NativeBindings::CreateFilterFor(filename)) | rx::operators::first() |
         rx::operators::map([](std::filesystem::path p) {
           return NativeBindings::LoadFrom(p);
         });
}
}  // namespace gel