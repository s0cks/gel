#ifndef GEL_NAMESPACE_H
#define GEL_NAMESPACE_H

#include <type_traits>
#include <vector>

#include "gel/argument.h"
#include "gel/common.h"
#include "gel/constructor.h"
#include "gel/expression.h"
#include "gel/object.h"
#include "gel/pointer.h"
#include "gel/type_traits.h"

namespace gel {
class Namespace;
using NamespaceList = std::vector<Namespace*>;
DECLARE_VISITOR(Namespace);

class Macro;
class MacroVisitor;
class Procedure;
class ProcedureVisitor;
class Namespace : public Object {
  friend class Script;
  friend class Parser;
  friend class Module;

 public:
  using Predicate = std::function<bool(Namespace*)>;
  static constexpr const auto kPrefixChar = '/';

  static inline auto IsNamed(Symbol* rhs) -> Predicate {
    ASSERT(rhs);
    return [rhs](Namespace* ns) {
      ASSERT(ns);
      return ns->GetName() == rhs->GetFullyQualifiedName();
    };
  }

  static inline auto IsNamed(const std::string& name) -> Predicate {
    ASSERT(!name.empty());
    return [&name](Namespace* ns) {
      ASSERT(ns);
      return ns->GetName() == name;
    };
  }

 private:
  Object* owner_ = nullptr;
  Symbol* symbol_;
  LocalScope* scope_;
  String* docs_ = nullptr;
  Constructor* init_ = nullptr;
  Array<Procedure*>* procedures_;
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
    procedures_ = Array<Procedure*>::New();
    ASSERT(procedures_);
  }

  void SetDocs(String* rhs) {
    ASSERT(rhs);
    docs_ = rhs;
  }

  void SetOwner(Object* rhs) {
    ASSERT(rhs);
    owner_ = rhs;
  }

  void SetInit(Constructor* rhs) {
    ASSERT(rhs);
    init_ = rhs;
  }

  auto IsKernelNamespace() const -> bool;
  auto VisitPointerPointers(PointerPointerVisitor* vis) -> bool override;

  void AddChild(Object* rhs) override;

 public:
  ~Namespace() override = default;

  auto GetSymbol() const -> Symbol* {
    return symbol_;
  }

  auto GetInit() const -> Constructor* {
    return init_;
  }

  inline auto HasInit() const -> bool {
    return GetInit() != nullptr;
  }

  auto GetScope() const -> LocalScope* {
    return scope_;
  }

  auto GetDocs() const -> String* {
    return docs_;
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

  auto GetProcedures() const -> Array<Procedure*>* {
    return procedures_;
  }

  auto InitNamespace() -> Namespace*;
  auto Get(Symbol* rhs) const -> Object*;
  auto Get(const std::string& rhs) const -> Object*;
  auto HasSymbol(Symbol* rhs) const -> bool;
  auto HasSymbol(const std::string& rhs) const -> bool;
  auto GetName() const -> const std::string&;
  auto CreateSymbol(const std::string& value) -> Symbol*;

  auto FindMacro(const std::string& name) -> Macro*;
  auto FindProcedure(const std::string& name) -> Procedure*;
  auto FindLambda(const std::string& name) -> Lambda*;
  auto FindNativeProcedure(const std::string& name) -> NativeProcedure*;

  auto VisitAllMacros(MacroVisitor* vis) const -> bool;
  auto VisitAllProcedures(ProcedureVisitor* vis) const -> bool;
  auto VisitAllNativeProcedures(ProcedureVisitor* vis) const -> bool;
  auto VisitAllLambdaProcedures(ProcedureVisitor* vis) const -> bool;
  DECLARE_TYPE(Namespace);

 private:
  static void Init();

 public:
  static auto New(Symbol* symbol, LocalScope* scope) -> Namespace*;
  static auto VisitAllNamespaces(NamespaceVisitor* vis) -> bool;
  static auto CreateConstructor(Namespace* ns, expr::SeqExpr* body = nullptr) -> Constructor*;
  static auto FindNamespace(const Predicate& filter) -> Namespace*;

  static inline auto FindNamespace(const std::string& name) -> Namespace* {
    return FindNamespace(IsNamed(name));
  }

  static inline auto FindNamespace(Symbol* rhs) -> Namespace* {
    ASSERT(rhs);
    return FindNamespace(IsNamed(rhs));
  }
};

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
