#ifndef GEL_MODULE_H
#define GEL_MODULE_H

#include <algorithm>
#include <filesystem>

#include "gel/namespace.h"
#include "gel/object.h"

namespace gel {
class Module;
using ModuleList = std::vector<Module*>;
class Module : public Object {
 private:
  String* name_;
  LocalScope* scope_;
  NamespaceList namespaces_{};

  void Append(Namespace* ns) {
    ASSERT(ns);
    namespaces_.push_back(ns);
  }

 protected:
  explicit Module(String* name, LocalScope* scope) :
    Object(),
    name_(name),
    scope_(scope) {
    ASSERT(name_);
    ASSERT(scope_);
  }

 public:
  ~Module() override = default;

  auto GetName() const -> String* {
    return name_;
  }

  auto GetScope() const -> LocalScope* {
    return scope_;
  }

  auto GetNamespaces() const -> const NamespaceList& {
    return namespaces_;
  }

  auto GetNumberOfNamespaces() const -> uword {
    return namespaces_.size();
  }

  auto GetNamespaceAt(const uword idx) const -> Namespace* {
    ASSERT(idx >= 0 && idx <= GetNumberOfNamespaces());
    return namespaces_[idx];
  }

  DECLARE_TYPE(Module);

 private:
  static inline auto IsNamed(std::string name) -> std::function<bool(Module*)> {
    return [name](Module* m) {
      ASSERT(m);
      return m && name == m->GetName()->Get();
    };
  }

 public:
  static auto IsLoaded(const std::string& name) -> bool;
  static auto Find(const std::string& name) -> Module*;
  static auto New(String* name, LocalScope* scope) -> Module*;
  static auto LoadFrom(const std::filesystem::path& abs_path) -> Module*;
  static auto VisitModules(const std::function<bool(Module*)>& vis) -> bool;
  static auto VisitModulePointers(const std::function<bool(Pointer**)>& vis) -> bool;
};
}  // namespace gel

#endif  // GEL_MODULE_H
