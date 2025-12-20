#ifndef GEL_MODULE_H
#define GEL_MODULE_H

#include <algorithm>
#include <filesystem>

#include "common.h"
#include "expr/expression.h"
#include "macro.h"
#include "namespace.h"
#include "object.h"
#include "pointer.h"

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
DECLARE_VISITOR_WRAPPER(Module, Module);

class ModuleLoader;
class Module : public Object {
  friend class Parser;
  friend class Runtime;  // TODO: revoke
  friend class BaseModuleLoader;
  friend class KernelModuleLoader;

 public:
  using Predicate = std::function<bool(Module*)>;

 private:
  LocalScope* scope_;
  Lambda* init_ = nullptr;
  Array<Namespace*>* namespaces_ = nullptr;
  ModuleLoader* loader_ = nullptr;

  static inline auto CreateDefaultNamespace(Module* m) -> Namespace* {
    ASSERT(m && m->HasSymbol());
    const auto ns = Namespace::New(m->GetSymbol(), LocalScope::New());
    ASSERT(ns);
    ns->SetOwner(m);
    return ns;
  }

  static inline auto CreateDefaultNamespaces(Module* m) -> Array<Namespace*>* {
    const auto namespaces = Array<Namespace*>::New();
    ASSERT(namespaces);
    namespaces->Push(CreateDefaultNamespace(m));
    return namespaces;
  }

 protected:
  explicit Module(Symbol* sym, LocalScope* scope) :
    Object(),
    scope_(scope) {
    ASSERT(scope_);
    SetSymbol(sym);
    SetInitialized(false);
    SetKernel(false);
    SetNamespaces(CreateDefaultNamespaces(this));
  }

  void SetLoader(ModuleLoader* rhs) {
    ASSERT(rhs);
    loader_ = rhs;
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

  void SetSymbol(Symbol* rhs) {
    ASSERT(rhs);
    ASSERT(kSymbolField);
    SetField(kSymbolField, rhs);
  }

  void SetNamespaces(Array<Namespace*>* rhs) {
    ASSERT(rhs);
    namespaces_ = rhs;
  }

  inline void ClearInitialized() {
    return SetInitialized(false);
  }

  auto VisitPointers(PointerVisitor* vis) -> bool override;
  auto VisitPointerPointers(PointerPointerVisitor* vis) -> bool override;

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

  auto GetLoader() const -> ModuleLoader* {
    return loader_;
  }

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

  auto GetSymbol() const -> Symbol* {
    ASSERT(kSymbolField);
    return GetField(kSymbolField)->AsSymbol();
  }

  auto GetScope() const -> LocalScope* {
    return scope_;
  }

  auto FindNamespace(const std::string& name) const -> Namespace* {
    return namespaces_ ? namespaces_->FindIf(IsNamed<Namespace>(name)) : nullptr;
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

  auto Init(Runtime* runtime) -> bool;

  inline auto HasSymbol() const -> bool {
    return GetSymbol() != nullptr;
  }

  auto IsKernel() const -> bool {
    return GetKernelField()->Get();
  }

  DECLARE_TYPE(Module);

 private:
  static Field* kKernelField;
  static Field* kFieldInitialized;
  static Field* kSymbolField;

 public:
  static void Init();
  static void GetAllLoadedModules(std::vector<Module*>& modules);
  static auto Find(const std::string& name) -> Module*;
  static auto New(Symbol* name, LocalScope* scope) -> Module*;
  static auto CreateConstructor(Module* rhs, expr::SeqExpr* body = nullptr) -> Lambda*;
  static auto FindOrLoad(const std::string& name) -> Module*;
  static auto LoadFrom(const std::filesystem::path& abs_path) -> Module*;
  static auto VisitAllModules(ModuleVisitor* vis) -> bool;
  static auto VisitAllModulePointers(PointerVisitor* vis) -> bool;
  static auto VisitAllModulePointerPointers(PointerPointerVisitor* vis) -> bool;

  static inline auto IsLoaded(const std::string& name) -> bool {
    return Find(name) != nullptr;
  }
};
static_assert(WithSymbol<Module>);
static_assert(WithInit<Module>);

namespace proc {
_DECLARE_NATIVE_PROCEDURE(gel_get_modules, "get-modules");
_DECLARE_NATIVE_PROCEDURE(gel_get_module, "get-module");

#define _DECLARE_MODULE_PROCEDURE(Name, Sym) _DECLARE_NATIVE_PROCEDURE(module_##Name, "Module:" Sym);
#define DECLARE_MODULE_PROCEDURE(Name)       _DECLARE_MODULE_PROCEDURE(Name, #Name);

_DECLARE_MODULE_PROCEDURE(is_kernel, "is-kernel?");
_DECLARE_MODULE_PROCEDURE(get_namespaces, "get-namespaces");

#undef DECLARE_MODULE_PROCEDURE
#undef _DECLARE_MODULE_PROCEDURE
}  // namespace proc
}  // namespace gel

#endif  // GEL_MODULE_H
