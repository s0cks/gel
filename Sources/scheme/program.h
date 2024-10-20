#ifndef SCM_PROGRAM_H
#define SCM_PROGRAM_H

#include <utility>

#include "scheme/common.h"
#include "scheme/expression.h"

namespace scm {
class Program {
  friend class Parser;
  DEFINE_NON_COPYABLE_TYPE(Program);

 private:
  ExpressionList expressions_;

 protected:
  explicit Program(const ExpressionList& expressions) :  // NOLINT(modernize-pass-by-value)
    expressions_(expressions) {}

  inline void Append(Expression* expr) {
    ASSERT(expr);
    expressions_.push_back(expr);
  }

 public:
  ~Program() = default;

  auto GetExpressions() const -> const ExpressionList& {
    return expressions_;
  }

  auto GetNumberOfExpressions() const -> uint64_t {
    return expressions_.size();
  }

  auto GetExpressionAt(const uint64_t idx) const -> Expression* {
    ASSERT(idx >= 0 && idx <= GetNumberOfExpressions());
    return expressions_[idx];
  }

  auto ToString() const -> std::string;
  auto Accept(ExpressionVisitor* vis) -> bool;
  auto VisitExpressions(ExpressionVisitor* vis) -> bool;

 public:
  static inline auto New(const ExpressionList& expressions = {}) -> Program* {
    return new Program(expressions);
  }
};
}  // namespace scm

#endif  // SCM_PROGRAM_H
