#ifndef GEL_MACRO_H
#define GEL_MACRO_H

#include <cstdint>
#include <functional>
#include <string>
#include <type_traits>

#include "gel/argument.h"
#include "gel/array.h"
#include "gel/common.h"
#include "gel/expression.h"
#include "gel/native_procedure.h"
#include "gel/object.h"
#include "gel/symbol.h"
#include "gel/type_traits.h"

namespace gel {
class Macro;
class Parser;
DECLARE_VISITOR(Macro);
class Macro : public Object {
  friend class Script;
  friend class Parser;
  friend class Module;
  friend class Namespace;

 public:
  using Predicate = std::function<bool(Macro*)>;

  template <typename T>
  static inline auto IsNamed(T value, std::enable_if_t<gel::is_string_like<T>::value>* = nullptr) -> Predicate {
    return [value](Macro* macro) {
      ASSERT(macro);
      return macro->GetSymbol()->Equals(value);
    };
  }

 private:
  Object* owner_ = nullptr;
  Symbol* symbol_ = nullptr;
  String* docstring_ = nullptr;
  LocalScope* scope_ = nullptr;
  Array<Argument*>* args_ = nullptr;
  expr::ExpressionList body_{};  // TODO: convert to array type

 protected:
  Macro() = default;
  Macro(Symbol* symbol, Array<Argument*>* args, const expr::ExpressionList& body) :  // NOLINT(modernize-pass-by-value)
    symbol_(symbol),
    args_(args),
    body_(body) {
    ASSERT(symbol);
  }

  void SetSymbol(Symbol* rhs) {
    ASSERT(rhs);
    symbol_ = rhs;
  }

  void SetOwner(Object* rhs) {
    ASSERT(rhs);
    owner_ = rhs;
  }

  void SetScope(LocalScope* rhs) {
    ASSERT(rhs);
    scope_ = rhs;
  }

  void SetArgs(Array<Argument*>* rhs) {
    ASSERT(rhs);
    args_ = rhs;
  }

  void SetBody(const expr::ExpressionList& rhs) {
    body_ = rhs;
  }

  void SetDocs(String* rhs) {
    ASSERT(rhs);
    docstring_ = rhs;
  }

  auto VisitPointers(PointerVisitor* vis) -> bool override;

 public:
  ~Macro() override = default;

  auto GetOwner() const -> Object* {
    return owner_;
  }

  inline auto HasOwner() const -> bool {
    return GetOwner() != nullptr;
  }

  auto GetSymbol() const -> Symbol* {
    return symbol_;
  }

  auto GetDocstring() const -> String* {
    return docstring_;
  }

  inline auto HasDocstring() const -> bool {
    return GetDocstring() != nullptr;
  }

  auto GetArgs() const -> Array<Argument*>* {
    return args_;
  }

  auto GetNumberOfArgs() const -> uint64_t {
    ASSERT(args_);
    return args_->GetLength();
  }

  inline auto HasArgs() const -> bool {
    return GetArgs() && GetNumberOfArgs() > 0;
  }

  auto GetArgAt(const uint64_t idx) const -> Argument* {
    ASSERT(idx >= 0 && idx <= GetNumberOfArgs());
    return args_->Get(idx);
  }

  auto GetBody() const -> const expr::ExpressionList& {
    return body_;
  }

  inline auto IsEmpty() const -> bool {
    return body_.empty();
  }

  inline auto GetArg(const std::string& name) const -> Argument* {
    return args_ ? args_->FindIf(Argument::IsNamed(name)) : nullptr;
  }

  inline auto GetArg(String* name) const -> Argument* {
    ASSERT(name);
    return GetArg(name->Get());
  }

  inline auto GetArg(Symbol* name) const -> Argument* {
    ASSERT(name);
    return GetArg(name->GetFullyQualifiedName());
  }

  DECLARE_TYPE(Macro);

 private:
  static void Init();
  static inline auto New() -> Macro* {
    return new Macro();
  }

 public:
  static inline auto New(Symbol* symbol, Array<Argument*>* args = nullptr, const expr::ExpressionList& body = {})
      -> Macro* {
    return new Macro(symbol, args, body);
  }
};

namespace proc {
#define _DECLARE_MACRO_PROCEDURE(Name, Sym) _DECLARE_NATIVE_PROCEDURE(macro_##Name, "Macro:" Sym)
#define DECLARE_MACRO_PROCEDURE(Name)       _DECLARE_MACRO_PROCEDURE(Name, #Name);

_DECLARE_MACRO_PROCEDURE(get_owner, "get-owner");
_DECLARE_MACRO_PROCEDURE(get_symbol, "get-symbol");

#undef _DECLARE_NAMESPACE_PROCEDURE
#undef DECLARE_NAMESPACE_PROCEDURE
}  // namespace proc
}  // namespace gel

#endif  // GEL_MACRO_H
