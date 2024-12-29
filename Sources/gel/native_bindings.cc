#include "gel/native_bindings.h"

#include <glog/logging.h>

#include "gel/common.h"
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

auto NativeBindings::LoadFrom(const std::filesystem::path& p) -> int {
  NativeBinding binding(p);
  const auto name = binding.GetName();
  DVLOG(100) << "initializing " << name << " plugin....";
  const auto status = binding.Init();  // TODO: better lib open error detection
  VLOG_IF(1000, status != EXIT_SUCCESS) << "failed to initialize the " << name
                                        << " plugin, plugin returned with status: " << status;
  return status;
}
}  // namespace gel