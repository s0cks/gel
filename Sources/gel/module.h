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
  friend class ModuleLoader;

 private:
  LocalScope* scope_;
  Array<Namespace*>* namespaces_ = nullptr;
  Array<Macro*>* macros_ = nullptr;
  Array<Lambda*>* lambdas_ = nullptr;
  Lambda* init_ = nullptr;

 protected:
  explicit Module(String* name, LocalScope* scope) :
    Object(),
    scope_(scope) {
    ASSERT(scope_);
    SetName(name);
    SetNamespaces(Array<Namespace*>::New());
    SetMacros(Array<Macro*>::New());
    SetLambdas(Array<Lambda*>::New());
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

  void SetMacros(Array<Macro*>* rhs) {
    ASSERT(rhs);
    macros_ = rhs;
  }

  void SetLambdas(Array<Lambda*>* rhs) {
    ASSERT(rhs);
    lambdas_ = rhs;
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

 public:
  ~Module() override = default;

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

  auto GetMacros() const -> Array<Macro*>* {
    return macros_;
  }

  auto GetNumberOfMacros() const -> uint64_t {
    return macros_->GetLength();
  }

  auto GetLambdas() const -> Array<Lambda*>* {
    return lambdas_;
  }

  auto GetNumberOfLambdas() const -> uint64_t {
    return lambdas_->GetLength();
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
    return HasName() && GetName()->Equals("_kernel");
  }

  DECLARE_TYPE(Module);

 private:
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
}  // namespace gel

#endif  // GEL_MODULE_H
