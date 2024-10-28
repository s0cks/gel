#ifndef SCM_EXPRESSION_COMPILER_H
#define SCM_EXPRESSION_COMPILER_H

#include <string>

#include "scheme/expression.h"
#include "scheme/expression_compiled.h"

namespace scm {
class ExpressionCompiler {
  DEFINE_NON_COPYABLE_TYPE(ExpressionCompiler);

 private:
 public:
  ExpressionCompiler() = default;
  virtual ~ExpressionCompiler() = default;
  virtual auto CompileExpression(Expression* expr) -> CompiledExpression*;

 public:
  static inline auto Compile(Expression* expr) -> CompiledExpression* {
    ExpressionCompiler compiler;
    return compiler.CompileExpression(expr);
  }

  static auto Compile(const std::string& expr) -> CompiledExpression*;
};
}  // namespace scm

#endif  // SCM_EXPRESSION_COMPILER_H
