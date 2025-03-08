#ifndef GEL_MODULE_LOADER_H
#define GEL_MODULE_LOADER_H

#include <filesystem>

#include "gel/common.h"
#include "gel/module.h"

namespace gel {
class Module;
class ModuleLoader {
  DEFINE_NON_COPYABLE_TYPE(ModuleLoader);

 public:
  class Result {
    DEFINE_DEFAULT_COPYABLE_TYPE(Result);

   private:
    bool success;
    Object* result;

   public:
    Result() = default;
    explicit Result(const bool s, Module* m) :
      result(m),
      success(s) {
      ASSERT(m);
    }
    explicit Result(const bool s, Error* error) :
      result(error),
      success(s) {
      ASSERT(error);
    }
    explicit Result(const std::string& error) :
      Result(false, Error::New(error)) {}
    explicit Result(const std::stringstream& ss) :
      Result(ss.str()) {}
    ~Result() = default;

    auto GetResult() const -> Object* {
      return result;
    }

    auto GetModule() const -> Module* {
      return GetResult()->AsModule();
    }

    auto GetError() const -> Error* {
      return GetResult()->AsError();
    }

    auto IsSuccess() const -> bool {
      return success;
    }

    operator bool() const {
      return IsSuccess();
    }

    auto operator->() const -> Module* {
      return GetModule();
    }

    operator Module*() const {
      return GetModule();
    }

    friend auto operator<<(std::ostream& stream, const Result& rhs) -> std::ostream& {
      if (!rhs.IsSuccess())
        return stream << "error: " << rhs.GetError()->GetMessage()->Get();
      return stream << rhs.GetModule()->ToString();
    }
  };

 protected:
  static inline auto HasValidModuleExtension(const std::string& path) -> bool {
    return path.ends_with(".cl");
  }

  static inline auto IsValidModulePath(const std::filesystem::path& path) -> bool {
    return std::filesystem::exists(path) && std::filesystem::is_regular_file(path) && HasValidModuleExtension(path);
  }

  struct ModulePath {
    std::filesystem::path path;

    ModulePath(std::filesystem::path p) :
      path(p) {}

    auto GetModuleName() const -> std::string {
      const auto& filename = path.filename().string();
      const auto slashpos = filename.find_last_of('/');
      const auto name_start = slashpos != std::string::npos ? slashpos : 0;
      const auto dotpos = filename.find_first_of('.', name_start);
      ASSERT(dotpos != std::string::npos && dotpos > name_start);
      const auto name_length = dotpos - name_start;
      return filename.substr(name_start, name_length);
    }

    operator std::filesystem::path() const {
      return path;
    }

    operator bool() const {
      return IsValidModulePath(path);
    }

    friend auto operator<<(std::ostream& stream, const ModulePath& rhs) -> std::ostream& {
      return stream << rhs.path;
    }
  };

  static inline auto CannotLoadFrom(const ModulePath& path) -> Result {
    ASSERT(!path);
    std::stringstream ss;
    ss << "cannot load Module from: " << path;
    return ModuleLoader::Result(ss);
  }

  static inline auto FailedToLoadFrom(const std::string& module_name, const ModulePath& path, const bool kernel = false)
      -> Result {
    ASSERT(!module_name.empty());
    std::stringstream ss;
    ss << "failed to load new";
    if (kernel)
      ss << " kernel";
    ss << " Module `" << module_name << "` from: " << path;
    return Result(ss);
  }

  static inline auto FailedToInitialize(Module* module, const ModulePath& path, const bool kernel) -> Result {
    ASSERT(module);
    std::stringstream ss;
    ss << "failed to initialize";
    if (kernel)
      ss << " kernel";
    ss << " Module `" << module->ToString() << "` from: " << path;
    return Result(ss);
  }

 protected:
  ModuleLoader() = default;

 public:
  virtual ~ModuleLoader() = default;
  virtual auto IsKernel() const -> bool = 0;
  virtual auto LoadModule(std::string path) -> Result = 0;
};

class BaseModuleLoader : public ModuleLoader {
  DEFINE_NON_COPYABLE_TYPE(BaseModuleLoader);

 private:
  std::filesystem::path root_;

 protected:
  explicit BaseModuleLoader(const std::filesystem::path& root) :
    root_(root) {
    ASSERT(std::filesystem::exists(root_) && std::filesystem::is_directory(root_));
  }

  auto GetModulePath(const std::string& path) const -> std::filesystem::path {
    return GetRoot() / path;
  }

  auto LoadAndInitialize(const ModulePath& module_path) -> Result;

 public:
  ~BaseModuleLoader() override = default;

  auto GetRoot() const -> const std::filesystem::path& {
    return root_;
  }
};

template <const bool Kernel>
class TemplateModuleLoader : public BaseModuleLoader {
  DEFINE_NON_COPYABLE_TYPE(TemplateModuleLoader);

 protected:
  explicit TemplateModuleLoader(const std::filesystem::path& root) :
    BaseModuleLoader(root) {}

 public:
  ~TemplateModuleLoader() override = default;

  auto IsKernel() const -> bool override {
    return Kernel;
  }
};

class KernelModuleLoader : public TemplateModuleLoader<true> {
  DEFINE_NON_COPYABLE_TYPE(KernelModuleLoader);

 private:
  explicit KernelModuleLoader(const std::filesystem::path& root) :
    TemplateModuleLoader<true>(root) {}

 public:
  ~KernelModuleLoader() override = default;
  auto LoadModule(std::string path) -> Result override;

 public:
  static void Init();
};

class ThreadModuleLoader : public TemplateModuleLoader<false> {
  DEFINE_NON_COPYABLE_TYPE(ThreadModuleLoader);

 private:
  explicit ThreadModuleLoader(const std::filesystem::path& root) :
    TemplateModuleLoader<false>(root) {}

 public:
  ~ThreadModuleLoader() override = default;
  auto LoadModule(std::string path) -> Result override;

 public:
  static void Init();
};

auto GetThreadKernelModuleLoader() -> KernelModuleLoader*;
auto GetThreadModuleLoader() -> ModuleLoader*;
}  // namespace gel

#endif  // GEL_MODULE_LOADER_H
