#ifndef SCM_EXPRESSION_COMPILER_H
#define SCM_EXPRESSION_COMPILER_H

#include <string>

#include "scheme/expression.h"

namespace scm {
class FlowGraph;
class ExpressionCompiler {
  DEFINE_NON_COPYABLE_TYPE(ExpressionCompiler);

 private:
  LocalScope* locals_;

 public:
  explicit ExpressionCompiler(LocalScope* locals) :
    locals_(locals) {}

  auto GetScope() const -> LocalScope* {
    return locals_;
  }

  virtual ~ExpressionCompiler() = default;
  virtual auto CompileExpression(Expression* expr) -> FlowGraph*;

 public:
  static inline auto Compile(Expression* expr, LocalScope* locals) -> FlowGraph* {
    ExpressionCompiler compiler(locals);
    return compiler.CompileExpression(expr);
  }

  static auto Compile(const std::string& expr, LocalScope* locals) -> FlowGraph*;
};
}  // namespace scm

#endif  // SCM_EXPRESSION_COMPILER_H
