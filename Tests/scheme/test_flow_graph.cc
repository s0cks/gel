#include <gtest/gtest.h>

#include "scheme/flow_graph.h"
#include "scheme/flow_graph_builder.h"
#include "scheme/flow_graph_dot.h"
#include "scheme/instruction.h"
#include "scheme/parser.h"

namespace scm {
using namespace ::testing;

class FlowGraphTest : public Test {};

TEST_F(FlowGraphTest, Test_Builder) {  // NOLINT
  const auto expr = Parser::ParseExpr("(begin (define test #t) (+ 99 1))");
  FlowGraphBuilder builder(expr);
  const auto flow_graph = builder.BuildGraph();
  ASSERT_TRUE(flow_graph);
  ASSERT_TRUE(flow_graph->GetEntry());
#ifdef SCM_DEBUG
  {
    const auto dot_graph = FlowGraphToDotGraph::BuildGraph("FlowGraph", flow_graph);
    ASSERT_TRUE(dot_graph);
    ASSERT_NO_FATAL_FAILURE(dot_graph->RenderPngToFilename("/Users/tazz/Projects/scheme/flow_graph.png"));  // NOLINT
    ASSERT_NO_FATAL_FAILURE(dot_graph->RenderToStdout());                                                   // NOLINT
  }
#endif  // SCM_DEBUG
}
}  // namespace scm