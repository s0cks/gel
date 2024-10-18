#include <gtest/gtest.h>

#include "scheme/flow_graph.h"
#include "scheme/flow_graph_builder.h"
#include "scheme/interpreter.h"
#include "scheme/parser.h"

namespace scm {
using namespace ::testing;

class InterpreterTest : public Test {};

TEST_F(InterpreterTest, Test_Execute) {  // NOLINT
  ByteTokenStream stream("(+ 10 (+ 10 10))");
  Parser parser(stream);
  const auto program = parser.ParseProgram();
  ASSERT_TRUE(program);
  FlowGraphBuilder builder(program);
  const auto flow_graph = builder.BuildGraph();
  ASSERT_TRUE(flow_graph);
  ASSERT_TRUE(flow_graph->GetEntry());

  DLOG(INFO) << "instructions:";
  InstructionIterator iter(flow_graph->GetEntry());
  while (iter.HasNext()) {
    const auto next = iter.Next();
    ASSERT_TRUE(next);
    DLOG(INFO) << "- " << next->ToString();
  }

  Interpreter interpreter;
  const auto result = interpreter.Execute(flow_graph->GetEntry());
  ASSERT_TRUE(result);
  DLOG(INFO) << "result: " << result->ToString();
}
}  // namespace scm