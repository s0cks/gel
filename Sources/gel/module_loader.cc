#include "module_loader.h"

#include <filesystem>

#include "common.h"
#include "local_scope.h"
#include "module.h"
#include "parser.h"
#include "runtime.h"
#include "thread_local.h"

namespace gel {
using Result = ModuleLoader::Result;

auto KernelModuleLoader::LoadModule(std::string path) -> Result {
  ASSERT(!path.empty());
  ModulePath module_path = GetModulePath(path);
  if (!module_path)
    return CannotLoadFrom(module_path);
  DVLOG(1) << "loading kernel Module from: " << module_path << "....";
  return LoadAndInitialize(module_path);
}

static ThreadLocal<KernelModuleLoader> kKernelModuleLoader;
static ThreadLocal<ModuleLoader> kModuleLoader;

void KernelModuleLoader::Init() {
  const auto& home = GetHomeEnvVar();
  LOG_IF(FATAL, !home) << "cannot initialize thread KernelModuleLoader, cannot find " << home.name() << " environment variable.";
  kKernelModuleLoader.Set(new KernelModuleLoader(home.path().value()));
}

auto BaseModuleLoader::LoadAndInitialize(const ModulePath& module_path) -> Result {
  ASSERT(module_path);
  const auto name = module_path.GetModuleName();
  const auto new_module = Parser::ParseModuleFrom(module_path.path, LocalScope::New(GetRuntime()->GetInitScope()), this);
  if (!new_module)
    return FailedToLoadFrom(name, module_path, IsKernel());
  ASSERT(new_module);
  new_module->SetKernel(IsKernel());
  if (new_module->HasInit()) {
    if (!new_module->Init(GetRuntime()))
      return FailedToInitialize(new_module, module_path, IsKernel());
    DVLOG(10) << new_module->ToString() << " is initialized!";
  }
  new_module->SetLoader(this);
  DVLOG(10) << new_module->ToString() << " is loaded!";
  return Result(true, new_module);
}

auto ThreadModuleLoader::LoadModule(std::string path) -> Result {
  ASSERT(!path.empty());
  ModulePath module_path = GetModulePath(path);
  if (!module_path)
    return CannotLoadFrom(module_path);
  DVLOG(1) << "loading kernel Module from: " << module_path << "....";
  return LoadAndInitialize(module_path);
}

auto GetThreadKernelModuleLoader() -> KernelModuleLoader* {
  ASSERT(kKernelModuleLoader);
  return kKernelModuleLoader.Get();
}

void ThreadModuleLoader::Init() {
  const auto& home = GetHomeEnvVar();
  LOG_IF(FATAL, !home) << "cannot initialize thread KernelModuleLoader, cannot find " << home.name() << " environment variable.";
  kModuleLoader.Set(new ThreadModuleLoader(home.path().value() / "lib"));
}

auto GetThreadModuleLoader() -> ModuleLoader* {
  ASSERT(kModuleLoader);
  return kModuleLoader.Get();
}
}  // namespace gel
