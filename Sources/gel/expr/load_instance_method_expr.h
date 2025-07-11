#ifndef GEL_LOAD_INSTANCE_METHOD_EXPR_H
#define GEL_LOAD_INSTANCE_METHOD_EXPR_H

#include "gel/expr/expression.h"

namespace gel::expr {
class LoadInstanceMethodExpr : Expression {
 private:
  Class* class_;
  Symbol* name_;

  LoadInstanceMethodExpr(Class* cls, Symbol* name) :
    Expression(),
    class_(cls),
    name_(name) {
    ASSERT(class_);
    ASSERT(name_);
  }

 public:
  ~LoadInstanceMethodExpr() override = default;

  auto GetTargetClass() const -> Class* {
    return class_;
  }

  auto GetTargetName() const -> Symbol* {
    return name_;
  }

  auto IsConstantExpr() const -> bool override;
  auto EvalToConstant(LocalScope* scope) const -> Object* override;
  DECLARE_EXPRESSION(LoadInstanceMethodExpr);

 public:
  static inline auto New(Class* cls, Symbol* name) -> LoadInstanceMethodExpr* {
    ASSERT(cls);
    return new LoadInstanceMethodExpr(cls, name);
  }
};
}  // namespace gel::expr

#endif  // GEL_LOAD_INSTANCE_METHOD_EXPR_H
