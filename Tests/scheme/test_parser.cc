#include <gtest/gtest.h>

#include <fstream>

#include "scheme/expression_dot.h"
#include "scheme/parser.h"

namespace scm {
using namespace ::testing;

class ParserTest : public Test {};

TEST_F(ParserTest, Test_Parse_Literal_True_Lowercase) {  // NOLINT
  const auto expr = Parser::ParseExpr("#t");
  ASSERT(expr);
}

TEST_F(ParserTest, Test_Parse_Literal_True_Uppercase) {  // NOLINT
  const auto expr = Parser::ParseExpr("#T");
  ASSERT(expr);
}

TEST_F(ParserTest, Test_Parse_Literal_Long) {  // NOLINT
  const auto expr = Parser::ParseExpr("1290172");
  ASSERT(expr);
}

TEST_F(ParserTest, Test_Parse_BinaryOp_Add_Long_Long) {  // NOLINT
  const auto expr = Parser::ParseExpr("(+ 10 10)");
  ASSERT(expr);
}

TEST_F(ParserTest, Test_Parse_VariableDef) {  // NOLINT
  const auto expr = Parser::ParseExpr("(define test #t)");
  ASSERT_TRUE(expr);
}

TEST_F(ParserTest, Test_Parse_BeginDef1) {  // NOLINT
  const auto expr = Parser::ParseExpr("(begin (define test #t))");
  ASSERT_TRUE(expr);
}

TEST_F(ParserTest, Test_Parse_BeginDef2) {  // NOLINT
  const auto expr = Parser::ParseExpr("(begin (define test #t) (define test2 #f) (define x (- (+ 99 1) (* 25 2))))");
  ASSERT_TRUE(expr);
  {
    const auto dot_graph = ExpressionToDot::BuildGraph("expr0", expr);
    ASSERT_TRUE(dot_graph);
    dot_graph->RenderToStdout();
    dot_graph->RenderPngToFilename("/Users/tazz/Projects/scheme/expr0_ast.png");
  }
}
}  // namespace scm