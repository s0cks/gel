#ifndef GEL_NAMESPACE_H
#define GEL_NAMESPACE_H

#include <type_traits>
#include <vector>

#include "gel/common.h"
#include "gel/expr/expression.h"
#include "gel/object.h"
#include "gel/pointer.h"
#include "gel/type_traits.h"

namespace gel {
class Namespace;
using NamespaceList = std::vector<Namespace*>;
DECLARE_VISITOR(Namespace);
class Namespace : public Object {
  friend class Script;
  friend class Parser;

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
  Procedure* init_ = nullptr;

 protected:
  explicit Namespace(Symbol* symbol, LocalScope* scope) :
    Object(),
    symbol_(symbol),
    scope_(scope) {
    ASSERT(symbol_);
    ASSERT(scope_);
  }

  void SetDocs(String* rhs) {
    ASSERT(rhs);
    docs_ = rhs;
  }

  void SetOwner(Object* rhs) {
    ASSERT(rhs);
    owner_ = rhs;
  }

  void SetInit(Procedure* rhs) {
    ASSERT(rhs);
    init_ = rhs;
  }

  auto CreateInit(const expr::ExpressionList& body = {}) -> Procedure*;
  auto IsKernelNamespace() const -> bool;
  auto VisitPointerPointers(PointerPointerVisitor* vis) -> bool override;

 public:
  ~Namespace() override = default;

  auto GetInit() const -> Procedure* {
    return init_;
  }

  inline auto HasInit() const -> bool {
    return GetInit() != nullptr;
  }

  auto GetSymbol() const -> Symbol* {
    return symbol_;
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

  auto InitNamespace() -> Namespace*;
  auto Get(Symbol* rhs) const -> Object*;
  auto Get(const std::string& rhs) const -> Object*;
  auto HasSymbol(Symbol* rhs) const -> bool;
  auto HasSymbol(const std::string& rhs) const -> bool;
  auto GetName() const -> const std::string&;
  auto CreateSymbol(const std::string& value) -> Symbol*;
  DECLARE_TYPE(Namespace);

 private:
  static void Init();

 public:
  static auto New(Symbol* symbol, LocalScope* scope) -> Namespace*;
  static auto VisitAllNamespaces(NamespaceVisitor* vis) -> bool;
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

#define _DECLARE_NAMESPACE_PROCEDURE(Name, Sym) _DECLARE_NATIVE_PROCEDURE(event_emitter_##Name, "Namespace:" Sym)
#define DECLARE_NAMESPACE_PROCEDURE(Name)       _DECLARE_NAMESPACE_PROCEDURE(Name, #Name);

#undef _DECLARE_NAMESPACE_PROCEDURE
#undef DECLARE_NAMESPACE_PROCEDURE
}  // namespace proc
}  // namespace gel

#endif  // GEL_NAMESPACE_H
