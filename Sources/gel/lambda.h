#ifndef GEL_LAMBDA_H
#define GEL_LAMBDA_H

#include <fmt/base.h>

#include <set>

#include "gel/argument.h"
#include "gel/common.h"
#include "gel/expression.h"
#include "gel/object.h"
#include "gel/pointer.h"
#include "gel/procedure.h"

namespace gel {
namespace expr {
class LambdaExpr;
class Expression;
}  // namespace expr
namespace ir {
class GraphEntryInstr;
}  // namespace ir

class Lambda : public Procedure, public Executable {
  friend class FlowGraphCompiler;

 private:
  Object* owner_ = nullptr;
  Symbol* name_;
  String* docstring_ = nullptr;
  ArgumentSet args_;           // TODO: fails to copy during GC
  expr::ExpressionList body_;  // TODO: fails to copy during GC
#ifdef GEL_DEBUG
  uword compile_time_ = 0;

  void SetCompileTime(const uword rhs) {
    ASSERT(rhs > 0);
    compile_time_ = rhs;
  }
#endif  // GEL_DEBUG

 protected:
  Lambda(Symbol* name, ArgumentSet args, const expr::ExpressionList& body) :  // NOLINT(modernize-pass-by-value)
    name_(name),
    args_(args),
    body_(body) {}

  auto VisitPointers(PointerVisitor* vis) -> bool override;

 public:
  ~Lambda() override = default;

  auto GetOwner() const -> Object* {
    return owner_;
  }

  inline auto HasOwner() const -> bool {
    return GetOwner() != nullptr;
  }

  void SetOwner(Object* rhs) {
    ASSERT(rhs);
    owner_ = rhs;
  }

  auto GetName() const -> Symbol* {
    return name_;
  }

  inline auto HasName() const -> bool {
    return GetName() != nullptr;
  }

  void SetName(Symbol* rhs) {
    ASSERT(rhs);
    name_ = rhs;
  }

  auto GetDocstring() const -> String* {
    return docstring_;
  }

  inline auto HasDocstring() const -> bool {
    return GetDocstring() != nullptr;
  }

  void SetDocstring(String* rhs) {
    ASSERT(rhs);
    docstring_ = rhs;
  }

  auto GetArgs() const -> const ArgumentSet& {
    return args_;
  }

  auto GetBody() const -> const expr::ExpressionList& {
    return body_;
  }

  inline auto IsEmpty() const -> bool {
    return body_.empty();
  }

  auto GetNumberOfArgs() const -> uint64_t {
    return args_.size();
  }

#ifdef GEL_DEBUG
  auto GetCompileTimeNanos() const -> uword {
    return compile_time_;
  }
#endif  // GEL_DEBUG

  DECLARE_TYPE(Lambda);

 public:
  static inline auto New(Symbol* name, const ArgumentSet& args, const expr::ExpressionList& body) -> Lambda* {
    return new Lambda(name, args, body);
  }

  static inline auto New(const ArgumentSet& args, const expr::ExpressionList& body) -> Lambda* {
    return new Lambda(nullptr, args, body);
  }
};
}  // namespace gel

#endif  // GEL_LAMBDA_H
