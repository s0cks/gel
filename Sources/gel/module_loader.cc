#include "gel/module_loader.h"

#include <filesystem>

#include "gel/common.h"
#include "gel/local_scope.h"
#include "gel/runtime.h"

namespace gel {
namespace fs = std::filesystem;

auto GetGelPathEnvVar() -> const EnvironmentVariable& {
  static EnvironmentVariable kVar("GEL_PATH");
  return kVar;
}

auto ModuleLoader::LoadModule(const fs::path& p) -> Module* {
  ASSERT(fs::is_regular_file(p));
  const auto module_name = GetFilename(p);
  ASSERT(!module_name.empty());
  if (Module::IsLoaded(module_name)) {
    DVLOG(10) << "skipping loading duplicate Module named `" << module_name << "`";
    return nullptr;
  }
  DVLOG(10) << "loading the `" << module_name << "` Module....";
  const auto m = Module::LoadFrom(p);
  LOG_IF(ERROR, !m) << "failed to load the `" << module_name << "` Module from: " << p;
  if (m && m->HasInit())
    LOG_IF(ERROR, !m->Init(GetRuntime())) << "failed to initialize " << m << ".";
  return m;
}

auto DirModuleLoader::LoadAllModules() -> bool {
  for (const auto& entry : fs::directory_iterator(GetDir())) {
    if (!fs::is_regular_file(entry)) {
      DVLOG(1000) << "skipping: " << entry.path();
      continue;
    }
    const auto& path = entry.path();
    if (!HasGelExtension(path)) {
      DVLOG(1000) << "skipping: " << path;
      continue;
    }
    const auto m = LoadModule(path);
    if (!m)
      continue;
    DVLOG(10) << m << " loaded!";
    if (VLOG_IS_ON(100)) {
      DLOG(INFO) << m->GetName() << " Scope:";
      PRINT_SCOPE(INFO, m->GetScope());
    }
  }
  return true;
}
}  // namespace gel