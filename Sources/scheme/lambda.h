#ifndef SCM_LAMBDA_H
#define SCM_LAMBDA_H

#include <set>

#include "scheme/argument.h"
#include "scheme/common.h"
#include "scheme/object.h"
#include "scheme/procedure.h"

namespace scm {
namespace expr {
class LambdaExpr;
class Expression;
}  // namespace expr
class Lambda : public Procedure {
 private:
  ArgumentSet args_;
  expr::LambdaExpr* expr_;
  CompiledExpression* compiled_ = nullptr;

 protected:
  explicit Lambda(ArgumentSet args, expr::LambdaExpr* body);

 public:
  auto GetArgs() const -> const ArgumentSet& {
    return args_;
  }

  auto GetExpression() const -> expr::LambdaExpr* {
    return expr_;
  }

  auto GetNumberOfArgs() const -> uint64_t {
    return args_.size();
  }

  auto Apply(Runtime* state) -> bool override;
  DECLARE_TYPE(Lambda);

 public:
  static inline auto New(const ArgumentSet& args, expr::LambdaExpr* body) -> Lambda* {
    return new Lambda(args, body);
  }
};
}  // namespace scm

#endif  // SCM_LAMBDA_H
