#include "scheme/expression_compiler.h"

#include "scheme/expression_dot.h"
#include "scheme/flags.h"
#include "scheme/flow_graph_builder.h"
#include "scheme/flow_graph_dot.h"
#include "scheme/instruction.h"
#include "scheme/parser.h"
#include "scheme/runtime.h"

namespace scm {
auto ExpressionCompiler::CompileExpression(Expression* expr) -> CompiledExpression* {
  ASSERT(expr);
#ifdef SCM_DEBUG
  if (FLAGS_dump_ast) {
    const auto dotgraph = expr::ExpressionToDot::BuildGraph("expr", expr);
    ASSERT(dotgraph);
    dotgraph->RenderPngToFilename(GetReportFilename("exec_expr_ast.png"));
  }
#endif  // SCM_DEBUG
  const auto flow_graph = FlowGraphBuilder::Build(expr);
  ASSERT(flow_graph);
  ASSERT(flow_graph->HasEntry());
#ifdef SCM_DEBUG
  if (FLAGS_dump_flow_graph) {
    const auto dotgraph = FlowGraphToDotGraph::BuildGraph("expr", flow_graph);
    ASSERT(dotgraph);
    dotgraph->RenderPngToFilename(GetReportFilename("exec_expr_flow_graph.png"));
  }
#endif  // SCM_DEBUG
  return CompiledExpression::New(flow_graph);
}

auto ExpressionCompiler::Compile(const std::string& expr) -> CompiledExpression* {
  ASSERT(!expr.empty());
  ByteTokenStream stream(expr);
  Parser parser(stream);
  return Compile(parser.ParseExpression());
}
}  // namespace scm