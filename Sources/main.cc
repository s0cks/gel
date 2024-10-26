#include <gflags/gflags.h>
#include <glog/logging.h>

#include <cstdlib>
#include <filesystem>
#include <iostream>

#include "scheme/expression_dot.h"
#include "scheme/flags.h"
#include "scheme/flow_graph_builder.h"
#include "scheme/flow_graph_dot.h"
#include "scheme/interpreter.h"
#include "scheme/lexer.h"
#include "scheme/parser.h"

using namespace scm;

static inline auto GetReportFilename(const std::string& filename) -> std::string {
  const auto reports_dir_flag = GetReportsDirFlag();
  const std::filesystem::path reports_dir =
      reports_dir_flag ? std::filesystem::path(*reports_dir_flag) : std::filesystem::current_path();
  return fmt::format("{0:s}/{1:s}", reports_dir.string(), filename);
}

auto main(int argc, char** argv) -> int {
  ::google::InitGoogleLogging(argv[0]);  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  ::google::ParseCommandLineFlags(&argc, &argv, true);

  Type::Init();

  const auto expr = GetExpressionFlag();
  if (expr) {
    DLOG(INFO) << "evaluating expression: " << (*expr);
    const auto program = Parser::Parse((*expr));
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
    const auto result = Interpreter::Eval(flow_graph);
    ASSERT(result);
    PrintValue(std::cout, result) << std::endl;
  }

  return EXIT_SUCCESS;
}