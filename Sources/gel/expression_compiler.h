#ifndef GEL_EXPRESSION_COMPILER_H
#define GEL_EXPRESSION_COMPILER_H

#include <string>

#include "gel/expression.h"

namespace gel {
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
}  // namespace gel

#endif  // GEL_EXPRESSION_COMPILER_H
