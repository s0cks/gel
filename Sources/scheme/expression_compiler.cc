#include "scheme/expression_compiler.h"

#include <units.h>

#include <chrono>

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
#ifdef SCM_DEBUG
  LOG(INFO) << "compiling expression: " << std::endl << expr;
  using Clock = std::chrono::high_resolution_clock;
  const auto start = Clock::now();
#endif  // SCM_DEBUG
  ByteTokenStream stream(expr);
  Parser parser(stream);
  const auto result = Compile(parser.ParseExpression());
#ifdef SCM_DEBUG
  const auto stop = Clock::now();
  const auto total_ns = std::chrono::duration_cast<std::chrono::milliseconds>((stop - start)).count();
  DLOG(INFO) << "expression compiled in " << units::time::millisecond_t(total_ns);  // NOLINT
#endif                                                                              // SCM_DEBUG
  return result;
}
}  // namespace scm