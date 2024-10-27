#include <gtest/gtest.h>

#include <fstream>

#include "scheme/expression_dot.h"
#include "scheme/parser.h"

namespace scm {
using namespace ::testing;

class ParserTest : public Test {};

TEST_F(ParserTest, Test_Parse_Literal_True_Lowercase) {  // NOLINT
  ByteTokenStream stream("#t");
  Parser parser(stream);
  const auto program = parser.ParseProgram();
  ASSERT_TRUE(program);
}

TEST_F(ParserTest, Test_Parse_Literal_True_Uppercase) {  // NOLINT
  ByteTokenStream stream("#T");
  Parser parser(stream);
  const auto program = parser.ParseProgram();
  ASSERT_TRUE(program);
}

TEST_F(ParserTest, Test_Parse_Literal_Long) {  // NOLINT
  ByteTokenStream stream("1290172");
  Parser parser(stream);
  const auto program = parser.ParseProgram();
  ASSERT_TRUE(program);
}

TEST_F(ParserTest, Test_Parse_BinaryOp_Add_Long_Long) {  // NOLINT
  ByteTokenStream stream("(+ 10 10)");
  Parser parser(stream);
  const auto program = parser.ParseProgram();
  ASSERT_TRUE(program);
}

TEST_F(ParserTest, Test_Parse_VariableDef) {  // NOLINT
  ByteTokenStream stream("(define test #t)");
  Parser parser(stream);
  const auto program = parser.ParseProgram();
  ASSERT_TRUE(program);
}

TEST_F(ParserTest, Test_Parse_BeginDef1) {  // NOLINT
  ByteTokenStream stream("(begin (define test #t))");
  Parser parser(stream);
  const auto program = parser.ParseProgram();
  ASSERT_TRUE(program);
}

TEST_F(ParserTest, Test_Parse_BeginDef2) {  // NOLINT
  ByteTokenStream stream("(begin (define test #t) (define test2 #f) (define x (- (+ 99 1) (* 25 2))))");
  Parser parser(stream);
  const auto program = parser.ParseProgram();
  ASSERT_TRUE(program);
  {
    const auto dot_graph = ExpressionToDot::BuildGraph("expr0", program->GetExpressionAt(0));
    ASSERT_TRUE(dot_graph);
    dot_graph->RenderToStdout();
    dot_graph->RenderPngToFilename("/Users/tazz/Projects/scheme/expr0_ast.png");
  }
}
}  // namespace scm