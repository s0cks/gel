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
  ByteTokenStream stream("(print #t)");
  Parser parser(stream);
  const auto program = parser.ParseProgram();
  ASSERT_TRUE(program);
  FlowGraphBuilder builder(program);
  const auto flow_graph = builder.BuildGraph();
  ASSERT_TRUE(flow_graph);
  ASSERT_TRUE(flow_graph->GetEntry());
#ifdef SCM_DEBUG
  {
    const auto dot_graph = FlowGraphToDotGraph::Build("FlowGraph", flow_graph);
    ASSERT_TRUE(dot_graph);
    ASSERT_NO_FATAL_FAILURE(dot_graph->RenderPngToFilename("/Users/tazz/Projects/scheme/flow_graph.png"));  // NOLINT
    ASSERT_NO_FATAL_FAILURE(dot_graph->RenderToStdout());                                                   // NOLINT
  }
#endif  // SCM_DEBUG
}
}  // namespace scm