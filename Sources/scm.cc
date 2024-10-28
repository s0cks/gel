#include <gflags/gflags.h>
#include <glog/logging.h>

#include <cstdlib>
#include <filesystem>
#include <iostream>

#include "scheme/expression_dot.h"
#include "scheme/flags.h"
#include "scheme/flow_graph_builder.h"
#include "scheme/flow_graph_dot.h"
#include "scheme/lexer.h"
#include "scheme/module_compiler.h"
#include "scheme/parser.h"
#include "scheme/runtime.h"

using namespace scm;

auto main(int argc, char** argv) -> int {
  ::google::InitGoogleLogging(argv[0]);  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  ::google::ParseCommandLineFlags(&argc, &argv, true);

  Type::Init();

  Module* m = nullptr;
  const auto module_flag = GetModuleFlag();
  if (module_flag) {
    const auto module_expr = Parser::ParseModule((*module_flag));
    ASSERT(module_expr);
    m = ModuleCompiler::Compile(module_expr);
  }

  const auto scope = Runtime::CreateInitScope();
  ASSERT(scope);
  if (m) {
#ifdef SCM_DEBUG
    LOG(INFO) << "Module Scope:";
    LocalScopePrinter::Print<>(m->GetScope(), __FILE__, __LINE__);
#endif  // SCM_DEBUG
    scope->Add(m->GetScope());
  }

  LocalScopePrinter::Print<>(scope, __FILE__, __LINE__);

  const auto expr = GetExpressionFlag();
  if (expr) {
    DLOG(INFO) << "evaluating expression: " << (*expr);
    ByteTokenStream stream((*expr));
    Parser parser(stream, scope);
    const auto program = parser.ParseProgram();
    ASSERT(program);
    if (FLAGS_dump_ast) {
      // TODO: merge program graphs
      const auto dot_graph = ExpressionToDot::BuildGraph("expr0", program->GetExpressionAt(0));
      ASSERT(dot_graph);
      // dot_graph->RenderToStdout();
      dot_graph->RenderPngToFilename(GetReportFilename("expr0_ast.png"));
    }
    const auto flow_graph = FlowGraphBuilder::Build(program);
    ASSERT(flow_graph);
    ASSERT(flow_graph->GetEntry());
    if (FLAGS_dump_flow_graph) {
      // TODO: merge program graphs
      const auto dot_graph = FlowGraphToDotGraph::BuildGraph("expr0", flow_graph);
      ASSERT(dot_graph);
      // dot_graph->RenderToStdout();
      dot_graph->RenderPngToFilename(GetReportFilename("expr0_flow_graph.png"));
    }
    const auto result = Runtime::EvalWithScope(flow_graph, scope);
    ASSERT(result);
    PrintValue(std::cout, result) << std::endl;
  }

  return EXIT_SUCCESS;
}