#include "gel/expression_compiler.h"

#include <units.h>

#include <chrono>

#include "gel/common.h"
#include "gel/expression.h"
#include "gel/expression_dot.h"
#include "gel/flags.h"
#include "gel/flow_graph_builder.h"
#include "gel/flow_graph_dot.h"
#include "gel/instruction.h"
#include "gel/macro.h"
#include "gel/parser.h"
#include "gel/runtime.h"

namespace gel {
class ExpressionLogger : public ExpressionVisitor {
  DEFINE_NON_COPYABLE_TYPE(ExpressionLogger);

 public:
  ExpressionLogger() = default;
  ~ExpressionLogger() override = default;

#define DEFINE_VISIT(Name)                      \
  auto Visit##Name(Name* expr)->bool override { \
    ASSERT(expr);                               \
    LOG(INFO) << expr->ToString();              \
    return true;                                \
  }
  FOR_EACH_EXPRESSION_NODE(DEFINE_VISIT)
#undef DEFINE_VISIT
};

auto ExpressionCompiler::CompileExpression(Expression* expr) -> FlowGraph* {
  ASSERT(expr);
#if defined(GEL_DEBUG) && defined(GEL_ENABLE_GV)
  if (FLAGS_dump_ast) {
    const auto dotgraph = expr::ExpressionToDot::BuildGraph("expr", expr);
    ASSERT(dotgraph);
    dotgraph->RenderPngToFilename(GetReportFilename("exec_expr_ast.png"));
  }
#endif  // defined(GEL_DEBUG) && defined(GEL_ENABLE_GV)

  const auto flow_graph = FlowGraphBuilder::Build(expr, GetScope());
  ASSERT(flow_graph);
  ASSERT(flow_graph->HasEntry());
#if defined(GEL_DEBUG) && defined(GEL_ENABLE_GV)
  if (FLAGS_dump_flow_graph) {
    const auto dotgraph = FlowGraphToDotGraph::BuildGraph("expr", flow_graph);
    ASSERT(dotgraph);
    dotgraph->RenderPngToFilename(GetReportFilename("exec_expr_flow_graph.png"));
  }
#endif  // defined(GEL_DEBUG) && defined(GEL_ENABLE_GV)
  return flow_graph;
}

auto ExpressionCompiler::Compile(const std::string& expr, LocalScope* locals) -> FlowGraph* {
  ASSERT(!expr.empty());
#ifdef GEL_DEBUG
  DVLOG(GEL_VLEVEL_1) << "compiling expression: " << std::endl << expr;
  using Clock = std::chrono::high_resolution_clock;
  const auto start = Clock::now();
#endif  // GEL_DEBUG

  const auto result = Compile(Parser::ParseExpr(expr, locals), locals);
  ASSERT(result);

#ifdef GEL_DEBUG
  const auto stop = Clock::now();
  const auto total_ns = std::chrono::duration_cast<std::chrono::milliseconds>((stop - start)).count();
  DVLOG(GEL_VLEVEL_1) << "expression compiled in " << units::time::millisecond_t(total_ns);  // NOLINT
#endif                                                                                       // GEL_DEBUG
  return result;
}
}  // namespace gel