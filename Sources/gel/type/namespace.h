#ifndef GEL_NAMESPACE_H
#define GEL_NAMESPACE_H

#include <type_traits>
#include <vector>

#include "gel/argument.h"
#include "gel/common.h"
#include "gel/frontend/expr/expr.h"
#include "gel/type/object.h"
#include "gel/heap/pointer.h"
#include "gel/type/type_traits.h"

namespace gel {
class Namespace;
using NamespaceList = std::vector<Namespace*>;

class Macro;
class MacroVisitor;
class Fn;
class FnVisitor;
class Namespace : public Object {
  friend class Script;
  friend class Parser;
  friend class Module;

 public:
  using Predicate = std::function<bool(Namespace*)>;
  static constexpr const auto kPrefixChar = '/';

 private:
  Object* owner_ = nullptr;
  Symbol* symbol_ = nullptr;
  LocalScope* scope_;
  Str* docs_ = nullptr;
  InitFn* init_ = nullptr;
  Array<Fn*>* procedures_;
  Array<Macro*>* macros_;

 protected:
  explicit Namespace(Symbol* symbol, LocalScope* scope) :
    Object(),
    symbol_(symbol),
    scope_(scope) {
    ASSERT(symbol_);
    ASSERT(scope_);
    macros_ = Array<Macro*>::New();
    ASSERT(macros_);
    procedures_ = Array<Fn*>::New();
    ASSERT(procedures_);
  }

  void SetOwner(Object* rhs) {
    ASSERT(rhs);
    owner_ = rhs;
  }

  void SetInit(InitFn* rhs) {
    ASSERT(rhs);
    init_ = rhs;
  }

  auto IsKernelNamespace() const -> bool;
  auto VisitPointerPointers(PointerPointerVisitor* vis) -> bool override;

  void AddChild(Object* rhs) override;

 public:
  ~Namespace() override = default;

  auto GetInit() const -> InitFn* {
    return init_;
  }

  inline auto HasInit() const -> bool {
    return GetInit() != nullptr;
  }

  auto Init(Runtime* runtime) -> bool;

  auto GetScope() const -> LocalScope* {
    return scope_;
  }

  auto GetDocstring() const -> Str* {
    return docs_;
  }

  inline auto HasDocstring() const -> bool {
    return GetDocstring() != nullptr;
  }

  void SetDocstring(Str* rhs) {
    ASSERT(rhs);
    docs_ = rhs;
  }

  auto GetOwner() const -> Object* {
    return owner_;
  }

  inline auto HasOwner() const -> bool {
    return GetOwner() != nullptr;
  }

  auto GetMacros() const -> Array<Macro*>* {
    return macros_;
  }

  auto GetFns() const -> Array<Fn*>* {
    return procedures_;
  }

  auto GetSymbol() const -> Symbol* {
    return symbol_;
  }

  inline auto HasSymbol() const -> bool {
    return GetSymbol() != nullptr;
  }

  void SetSymbol(Symbol* rhs) {
    ASSERT(rhs);
    symbol_ = rhs;
  }

  auto InitNamespace() -> Namespace*;
  auto Get(Symbol* rhs) const -> Object*;
  auto Get(const std::string& rhs) const -> Object*;

  auto HasSymbol(Symbol* rhs) const -> bool;
  auto HasSymbol(const std::string& rhs) const -> bool;
  auto CreateSymbol(const std::string& value) -> Symbol*;

  auto FindMacro(const std::string& name) -> Macro*;
  auto FindFn(const std::string& name) -> Fn*;
  auto FindLambdaFn(const std::string& name) -> LambdaFn*;
  auto FindNativeFn(const std::string& name) -> NativeFn*;

  template <VisitorLike<Macro*> Visitor>
  inline auto VisitAllMacros(Visitor& vis) const -> bool {
    return macros_->VisitAll(vis);
  }

  template <VisitorLike<Fn*> Visitor>
  inline auto VisitAllFns(Visitor& vis) const -> bool {
    return procedures_->VisitAll(vis);
  }

  template <VisitorLike<Fn*> Visitor>
  inline auto VisitAllNativeFns(Visitor& vis) const -> bool {
    return procedures_->VisitIf(vis, Fn::IsNativeProc);
  }

  template <VisitorLike<Fn*> Visitor>
  inline auto VisitAllLambdaFnFns(Visitor& vis) const -> bool {
    return procedures_->VisitIf(vis, Fn::IsLambdaFnProc);
  }

  DECLARE_TYPE(Namespace);

 private:
  static void Init();

 public:
  static auto New(Symbol* symbol, LocalScope* scope) -> Namespace*;

  template <VisitorLike<Namespace*> Visitor>
  static auto VisitAllNamespaces(Visitor& vis) -> bool;

  static auto CreateInitFn(Namespace* ns, expr::SeqExpr* body = nullptr) -> InitFn*;
  static auto FindNamespace(const Predicate& filter) -> Namespace*;

  static inline auto FindNamespace(const std::string& name) -> Namespace* {
    return FindNamespace(IsNamed<Namespace>(name));
  }

  static inline auto FindNamespace(Symbol* rhs) -> Namespace* {
    ASSERT(rhs);
    return FindNamespace(IsNamed<Namespace>(*rhs));
  }
};
static_assert(WithSymbol<Namespace>);
static_assert(HasDocstring<Namespace>);
static_assert(WithInit<Namespace>);

namespace proc {
_DECLARE_NATIVE_PROCEDURE(gel_get_namespace, "gel/get-namespace");
_DECLARE_NATIVE_PROCEDURE(gel_get_namespaces, "gel/get-namespaces");

#define _DECLARE_NAMESPACE_PROCEDURE(Name, Sym) _DECLARE_NATIVE_PROCEDURE(namespace_##Name, "Namespace:" Sym)
#define DECLARE_NAMESPACE_PROCEDURE(Name)       _DECLARE_NAMESPACE_PROCEDURE(Name, #Name);

_DECLARE_NAMESPACE_PROCEDURE(get_owner, "get-owner");
_DECLARE_NAMESPACE_PROCEDURE(get_symbol, "get-symbol");
_DECLARE_NAMESPACE_PROCEDURE(get_macros, "get-macros");
_DECLARE_NAMESPACE_PROCEDURE(get_procedures, "get-procedures");
_DECLARE_NAMESPACE_PROCEDURE(get_lambdas, "get-lambdas");
_DECLARE_NAMESPACE_PROCEDURE(get_native_procedures, "get-native-procedures");

#undef _DECLARE_NAMESPACE_PROCEDURE
#undef DECLARE_NAMESPACE_PROCEDURE
}  // namespace proc
}  // namespace gel

#endif  // GEL_NAMESPACE_H
