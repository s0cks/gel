#include <gtest/gtest.h>

#include "gel/flow_graph.h"
#include "gel/flow_graph_builder.h"
#include "gel/flow_graph_dot.h"
#include "gel/instruction.h"
#include "gel/parser.h"

namespace gel {
using namespace ::testing;

class FlowGraphTest : public Test {};

TEST_F(FlowGraphTest, Test_Builder) {  // NOLINT
  const auto expr = Parser::ParseExpr("(begin (define test #t) (+ 99 1))");
  const auto scope = LocalScope::New();
  const auto flow_graph = FlowGraphBuilder::Build(expr, scope);
  ASSERT_TRUE(flow_graph);
  ASSERT_TRUE(flow_graph->GetEntry());
}
}  // namespace gel