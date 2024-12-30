#ifndef GEL_MODULE_H
#define GEL_MODULE_H

#include <algorithm>
#include <filesystem>

#include "gel/common.h"
#include "gel/expression.h"
#include "gel/namespace.h"
#include "gel/object.h"
#include "gel/pointer.h"

namespace gel {
class Module;
using MacroList = std::vector<Macro*>;
using ModuleList = std::vector<Module*>;
class Module : public Object {
  friend class Parser;
  friend class Runtime;  // TODO: revoke
  friend class ModuleLoader;

 private:
  String* name_;
  LocalScope* scope_;
  NamespaceList namespaces_{};
  MacroList macros_{};
  Lambda* init_ = nullptr;

  void Append(Namespace* ns);
  void Append(Macro* macro);
  auto CreateInitFunc(const expr::ExpressionList& body) -> Lambda*;

 protected:
  explicit Module(String* name, LocalScope* scope) :
    Object(),
    name_(name),
    scope_(scope) {
    ASSERT(name_);
    ASSERT(scope_);
  }

  void SetInit(Lambda* rhs) {
    ASSERT(rhs);
    init_ = rhs;
  }

  void SetInitialized(const bool rhs = true) {
    ASSERT(kFieldInitialized);
    SetField(kFieldInitialized, Bool::Box(rhs));
  }

  inline void ClearInitialized() {
    return SetInitialized(false);
  }

  auto Init(Runtime* runtime) -> bool;
  auto VisitPointers(PointerPointerVisitor* vis) -> bool override;

 public:
  ~Module() override = default;

  auto IsInitialized() const -> bool {
    ASSERT(kFieldInitialized);
    return GetField(kFieldInitialized)->AsBool()->Get();
  }

  auto GetName() const -> String* {
    return name_;
  }

  auto GetScope() const -> LocalScope* {
    return scope_;
  }

  auto GetNamespace(const std::string& name) const -> Namespace* {
    ASSERT(!name.empty());
    for (const auto& ns : namespaces_) {
      if (ns->GetName() == name)
        return ns;
    }
    return nullptr;
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

  auto GetInit() const -> Lambda* {
    return init_;
  }

  inline auto HasInit() const -> bool {
    return GetInit() != nullptr;
  }

  DECLARE_TYPE(Module);

 private:
  static Field* kFieldInitialized;
  static inline auto IsNamed(std::string name) -> std::function<bool(Module*)> {
    return [name](Module* m) {
      ASSERT(m);
      return m && name == m->GetName()->Get();
    };
  }

 public:
  static void GetAllLoadedModules(std::vector<Module*>& modules);
  static auto IsLoaded(const std::string& name) -> bool;
  static auto Find(const std::string& name) -> Module*;
  static auto New(String* name, LocalScope* scope) -> Module*;
  static auto LoadFrom(const std::filesystem::path& abs_path) -> Module*;
  static auto VisitModules(const std::function<bool(Module*)>& vis) -> bool;
  static auto VisitModulePointers(const std::function<bool(Pointer**)>& vis) -> bool;
  static auto FindOrLoad(const std::string& name) -> Module*;
};
}  // namespace gel

#endif  // GEL_MODULE_H
