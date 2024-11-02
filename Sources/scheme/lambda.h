#ifndef SCM_LAMBDA_H
#define SCM_LAMBDA_H

#include <set>

#include "scheme/argument.h"
#include "scheme/common.h"
#include "scheme/procedure.h"
#include "scheme/type.h"

namespace scm {
namespace expr {
class Expression;
}
class Lambda : public Procedure {
 private:
  ArgumentSet args_;
  expr::Expression* body_;
  CompiledExpression* expr_ = nullptr;

 protected:
  explicit Lambda(const ArgumentSet& args, expr::Expression* body);

 public:
  auto GetArgs() const -> const ArgumentSet& {
    return args_;
  }

  auto GetBody() const -> expr::Expression* {
    return body_;
  }

  auto GetNumberOfArgs() const -> uint64_t {
    return args_.size();
  }

  auto Apply(Runtime* state) const -> bool override;
  DECLARE_TYPE(Lambda);

 public:
  static inline auto New(const ArgumentSet& args, expr::Expression* body) -> Lambda* {
    return new Lambda(args, body);
  }
};
}  // namespace scm

#endif  // SCM_LAMBDA_H
