#ifndef GEL_MACRO_H
#define GEL_MACRO_H

#include "gel/argument.h"
#include "gel/common.h"
#include "gel/expr/expression.h"
#include "gel/object.h"

namespace gel {
class Parser;
class Macro : public Object {
  friend class Script;
  friend class Parser;
  friend class Module;

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
  static inline auto New() -> Macro* {
    return new Macro();
  }

 public:
  static inline auto New(Symbol* symbol, Array<Argument*>* args = nullptr, const expr::ExpressionList& body = {}) -> Macro* {
    return new Macro(symbol, args, body);
  }
};
}  // namespace gel

#endif  // GEL_MACRO_H
