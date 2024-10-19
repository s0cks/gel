#include <gtest/gtest.h>

#include "scheme/flow_graph.h"
#include "scheme/flow_graph_builder.h"
#include "scheme/flow_graph_dot.h"
#include "scheme/interpreter.h"
#include "scheme/parser.h"

namespace scm {
using namespace ::testing;

class InterpreterTest : public Test {};

TEST_F(InterpreterTest, Test_Execute) {  // NOLINT
  ByteTokenStream stream("(+ 10 10)");
  Parser parser(stream);
  const auto program = parser.ParseProgram();
  ASSERT_TRUE(program);
  FlowGraphBuilder builder(program);
  const auto flow_graph = builder.BuildGraph();
  ASSERT_TRUE(flow_graph);
  ASSERT_TRUE(flow_graph->GetEntry());

#ifdef SCM_DEBUG
  const auto dot_graph = FlowGraphToDotGraph::Build("FlowGraph", flow_graph);
  ASSERT_TRUE(dot_graph);
  ASSERT_NO_FATAL_FAILURE(dot_graph->RenderToStdout());  // NOLINT
  ASSERT_NO_FATAL_FAILURE(
      dot_graph->RenderPngToFilename("/Users/tazz/Projects/scheme/interpreter_test_execute_flowgraph.png"));  // NOLINT
#endif                                                                                                        // SCM_DEBUG

  Interpreter interpreter;
  const auto result = interpreter.Execute(flow_graph->GetEntry());
  ASSERT_TRUE(result);
  ASSERT_TRUE(result->IsNumber());
  ASSERT_EQ(result->AsNumber()->GetValue(), 20);
  DLOG(INFO) << "result: " << result->ToString();
}
}  // namespace scm