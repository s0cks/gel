#ifndef GEL_MODULE_LOADER_H
#define GEL_MODULE_LOADER_H

#include <filesystem>

#include "gel/common.h"
#include "gel/module.h"

namespace gel {
auto GetGelPathEnvVar() -> const EnvironmentVariable&;

class ModuleLoader {
  DEFINE_NON_COPYABLE_TYPE(ModuleLoader);

 protected:
  ModuleLoader() = default;
  auto LoadModule(const std::filesystem::path& path) -> Module*;

 public:
  virtual ~ModuleLoader() = default;
};

static inline auto HasGelExtension(const std::string& rhs) -> bool {
  return rhs.ends_with(".cl");
}

static inline auto HasGelExtension(const std::filesystem::path& rhs) -> bool {
  return HasGelExtension(rhs.string());
}

class DirModuleLoader : public ModuleLoader {
  DEFINE_NON_COPYABLE_TYPE(DirModuleLoader);

 private:
  std::filesystem::path dir_;

 public:
  explicit DirModuleLoader(const std::filesystem::path& dir) :
    ModuleLoader(),
    dir_(dir) {}
  explicit DirModuleLoader(const std::string& dir) :
    DirModuleLoader(std::filesystem::path(dir)) {}
  ~DirModuleLoader() override = default;

  auto GetDir() const -> const std::filesystem::path& {
    return dir_;
  }

  auto LoadAllModules() -> bool;
};
}  // namespace gel

#endif  // GEL_MODULE_LOADER_H
