#ifndef GEL_MODULE_H
#define GEL_MODULE_H

#include <algorithm>
#include <filesystem>

#include "gel/common.h"
#include "gel/expr/expression.h"
#include "gel/macro.h"
#include "gel/namespace.h"
#include "gel/object.h"
#include "gel/pointer.h"

namespace gel {
class Module;
using MacroList = std::vector<Macro*>;
using ModuleList = std::vector<Module*>;

class ModuleVisitor {
  DEFINE_NON_COPYABLE_TYPE(ModuleVisitor);

 protected:
  ModuleVisitor() = default;

 public:
  virtual ~ModuleVisitor() = default;
  virtual auto Visit(Module* m) -> bool = 0;
};
DECLARE_VISITOR_WRAPPER(Module, Module*);

class Module : public Object {
  friend class Parser;
  friend class Runtime;  // TODO: revoke
  friend class BaseModuleLoader;
  friend class KernelModuleLoader;

 private:
  LocalScope* scope_;
  Lambda* init_ = nullptr;
  Array<Namespace*>* namespaces_ = nullptr;

  static inline auto CreateDefaultNamespace(String* name) -> Namespace* {
    ASSERT(name);
    return Namespace::New(Symbol::New(name), LocalScope::New());
  }

  static inline auto CreateDefaultNamespaces(String* name) -> Array<Namespace*>* {
    const auto namespaces = Array<Namespace*>::New();
    ASSERT(namespaces);
    namespaces->Push(CreateDefaultNamespace(name));
    return namespaces;
  }

 protected:
  explicit Module(String* name, LocalScope* scope) :
    Object(),
    scope_(scope) {
    ASSERT(scope_);
    SetName(name);
    SetInitialized(false);
    SetKernel(false);
    SetNamespaces(CreateDefaultNamespaces(name));
  }

  void SetInit(Lambda* rhs) {
    ASSERT(rhs);
    init_ = rhs;
  }

  void SetInitialized(Bool* rhs) {
    ASSERT(rhs);
    ASSERT(kFieldInitialized);
    return SetField(kFieldInitialized, rhs);
  }

  void SetInitialized(const bool rhs = true) {
    ASSERT(kFieldInitialized);
    return SetInitialized(Bool::Box(rhs));
  }

  void SetName(String* rhs) {
    ASSERT(rhs);
    ASSERT(kNameField);
    SetField(kNameField, rhs);
  }

  void SetNamespaces(Array<Namespace*>* rhs) {
    ASSERT(rhs);
    namespaces_ = rhs;
  }

  inline void ClearInitialized() {
    return SetInitialized(false);
  }

  auto Init(Runtime* runtime) -> bool;
  auto VisitPointers(PointerVisitor* vis) -> bool override;
  auto VisitPointerPointers(PointerPointerVisitor* vis) -> bool override;
  auto CreateInitFunc(const expr::ExpressionList& body) -> Lambda*;

  void AddChild(Object* rhs) override;

  auto GetKernelField() const -> Bool* {
    ASSERT(kKernelField);
    return GetField(kKernelField)->AsBool();
  }

  void SetKernelField(Bool* rhs) {
    ASSERT(kKernelField);
    ASSERT(rhs);
    return SetField(kKernelField, rhs);
  }

  void SetKernel(const bool rhs) {
    return SetKernelField(Bool::Box(rhs));
  }

 public:
  ~Module() override = default;

  auto GetDefaultNamespace() const -> Namespace* {
    ASSERT(!namespaces_->IsEmpty());
    return namespaces_->Get(0);
  }

  auto GetInitialized() const -> Bool* {
    ASSERT(kFieldInitialized);
    return GetField(kFieldInitialized)->AsBool();
  }

  auto IsInitialized() const -> bool {
    return GetInitialized()->Get();
  }

  auto GetName() const -> String* {
    ASSERT(kNameField);
    return GetField(kNameField)->AsString();
  }

  auto GetScope() const -> LocalScope* {
    return scope_;
  }

  auto FindNamespace(const std::string& name) const -> Namespace* {
    return namespaces_ ? namespaces_->FindIf(Namespace::IsNamed(name)) : nullptr;
  }

  auto GetNamespaces() const -> Array<Namespace*>* {
    return namespaces_;
  }

  auto GetNumberOfNamespaces() const -> uword {
    return namespaces_->GetLength();
  }

  auto GetNamespaceAt(const uword idx) const -> Namespace* {
    ASSERT(idx >= 0 && idx <= GetNumberOfNamespaces());
    return namespaces_->Get(idx);
  }

  auto GetInit() const -> Lambda* {
    return init_;
  }

  inline auto HasInit() const -> bool {
    return GetInit() != nullptr;
  }

  inline auto HasName() const -> bool {
    ASSERT(kNameField);
    return GetName() != nullptr;
  }

  auto IsKernel() const -> bool {
    return GetKernelField()->Get();
  }

  DECLARE_TYPE(Module);

 private:
  static Field* kKernelField;
  static Field* kFieldInitialized;
  static Field* kNameField;
  static inline auto IsNamed(std::string name) -> std::function<bool(Module*)> {
    return [name](Module* m) {
      ASSERT(m);
      return m && name == m->GetName()->Get();
    };
  }

 public:
  static void Init();
  static void GetAllLoadedModules(std::vector<Module*>& modules);
  static auto Find(const std::string& name) -> Module*;
  static auto New(String* name, LocalScope* scope) -> Module*;
  static auto FindOrLoad(const std::string& name) -> Module*;
  static auto LoadFrom(const std::filesystem::path& abs_path) -> Module*;
  static auto VisitAllModules(ModuleVisitor* vis) -> bool;
  static auto VisitAllModulePointers(PointerVisitor* vis) -> bool;
  static auto VisitAllModulePointerPointers(PointerPointerVisitor* vis) -> bool;

  static inline auto IsLoaded(const std::string& name) -> bool {
    return Find(name) != nullptr;
  }
};

namespace proc {
_DECLARE_NATIVE_PROCEDURE(gel_get_modules, "gel/get-modules");
_DECLARE_NATIVE_PROCEDURE(gel_get_module, "gel/get-module");

_DECLARE_NATIVE_PROCEDURE(module_is_kernel, "Module:is-kernel?");
}  // namespace proc
}  // namespace gel

#endif  // GEL_MODULE_H
