#include "scheme/expression_compiler.h"

#include <units.h>

#include <chrono>

#include "scheme/common.h"
#include "scheme/expression.h"
#include "scheme/expression_dot.h"
#include "scheme/flags.h"
#include "scheme/flow_graph_builder.h"
#include "scheme/flow_graph_dot.h"
#include "scheme/instruction.h"
#include "scheme/macro.h"
#include "scheme/parser.h"
#include "scheme/runtime.h"

namespace scm {
class ExpressionLogger : public ExpressionVisitor {
  DEFINE_NON_COPYABLE_TYPE(ExpressionLogger);

 public:
  ExpressionLogger() = default;
  ~ExpressionLogger() override = default;

#define DEFINE_VISIT(Name)                        \
  auto Visit##Name(Name* expr) -> bool override { \
    ASSERT(expr);                                 \
    LOG(INFO) << expr->ToString();                \
    return true;                                  \
  }
  FOR_EACH_EXPRESSION_NODE(DEFINE_VISIT)
#undef DEFINE_VISIT
};

auto ExpressionCompiler::CompileExpression(Expression* expr) -> FlowGraph* {
  ASSERT(expr);
#if defined(SCM_DEBUG) && defined(SCM_ENABLE_GV)
  if (FLAGS_dump_ast) {
    const auto dotgraph = expr::ExpressionToDot::BuildGraph("expr", expr);
    ASSERT(dotgraph);
    dotgraph->RenderPngToFilename(GetReportFilename("exec_expr_ast.png"));
  }
#endif  // defined(SCM_DEBUG) && defined(SCM_ENABLE_GV)

  const auto flow_graph = FlowGraphBuilder::Build(expr, GetScope());
  ASSERT(flow_graph);
  ASSERT(flow_graph->HasEntry());
#if defined(SCM_DEBUG) && defined(SCM_ENABLE_GV)
  if (FLAGS_dump_flow_graph) {
    const auto dotgraph = FlowGraphToDotGraph::BuildGraph("expr", flow_graph);
    ASSERT(dotgraph);
    dotgraph->RenderPngToFilename(GetReportFilename("exec_expr_flow_graph.png"));
  }
#endif  // defined(SCM_DEBUG) && defined(SCM_ENABLE_GV)
  return flow_graph;
}

auto ExpressionCompiler::Compile(const std::string& expr, LocalScope* locals) -> FlowGraph* {
  ASSERT(!expr.empty());
#ifdef SCM_DEBUG
  DVLOG(SCM_VLEVEL_1) << "compiling expression: " << std::endl << expr;
  using Clock = std::chrono::high_resolution_clock;
  const auto start = Clock::now();
#endif  // SCM_DEBUG

  const auto result = Compile(Parser::ParseExpr(expr, locals), locals);
  ASSERT(result);

#ifdef SCM_DEBUG
  const auto stop = Clock::now();
  const auto total_ns = std::chrono::duration_cast<std::chrono::milliseconds>((stop - start)).count();
  DVLOG(SCM_VLEVEL_1) << "expression compiled in " << units::time::millisecond_t(total_ns);  // NOLINT
#endif                                                                                       // SCM_DEBUG
  return result;
}
}  // namespace scm