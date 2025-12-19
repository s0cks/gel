#ifndef GEL_BINDING_EXPR_H
#define GEL_BINDING_EXPR_H

#include "expr/expression.h"

namespace gel::expr {
class BindingExpr : public TemplateExpression<1> {
 private:
  LocalVariable* local_;

 protected:
  BindingExpr(LocalVariable* local, Expression* value) :
    local_(local) {
    ASSERT(local_);
    SetValue(value);
  }

  inline void SetValue(Expression* rhs) {
    ASSERT(rhs);
    SetChildAt(0, rhs);
  }

 public:
  ~BindingExpr() override = default;

  auto GetLocal() const -> LocalVariable* {
    return local_;
  }

  auto GetValue() const -> Expression* {
    return GetChildAt(0);
  }

  DECLARE_EXPRESSION(BindingExpr);

 public:
  static inline auto New(LocalVariable* local, Expression* value) -> BindingExpr* {
    return new BindingExpr(local, value);
  }
};

using BindingList = std::vector<BindingExpr*>;

static inline auto operator<<(std::ostream& stream, const BindingList& rhs) -> std::ostream& {
  stream << "[";
  auto remaining = rhs.size();
  for (const auto& value : rhs) {
    ASSERT(value);
    stream << value;
    if (--remaining > 0)
      stream << ", ";
  }
  stream << "]";
  return stream;
}
}  // namespace gel::expr

#endif  // GEL_BINDING_EXPR_H
