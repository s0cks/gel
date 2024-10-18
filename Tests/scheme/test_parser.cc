#include <gtest/gtest.h>

#include <fstream>

#include "scheme/ast_printer.h"
#include "scheme/ast_renderer.h"
#include "scheme/parser.h"

namespace scm {
using namespace ::testing;

class ParserTest : public Test {};

TEST_F(ParserTest, Test_Parse_Literal_True_Lowercase) {  // NOLINT
  ByteTokenStream stream("#t");
  Parser parser(stream);
  const auto program = parser.ParseProgram();
  ASSERT_TRUE(program);
  ast::AstPrinter::Print(program, __FILE__, __LINE__, google::INFO);
}

TEST_F(ParserTest, Test_Parse_Literal_True_Uppercase) {  // NOLINT
  ByteTokenStream stream("#T");
  Parser parser(stream);
  const auto program = parser.ParseProgram();
  ASSERT_TRUE(program);
  ast::AstPrinter::Print(program, __FILE__, __LINE__, google::INFO);
}

TEST_F(ParserTest, Test_Parse_VariableDef) {  // NOLINT
  ByteTokenStream stream("(define test #t)");
  Parser parser(stream);
  const auto program = parser.ParseProgram();
  ASSERT_TRUE(program);
  ast::AstPrinter::Print(program, __FILE__, __LINE__, google::INFO);
}

TEST_F(ParserTest, Test_Parse_BeginDef1) {  // NOLINT
  ByteTokenStream stream("(begin (define test #t))");
  Parser parser(stream);
  const auto program = parser.ParseProgram();
  ASSERT_TRUE(program);
  ast::AstPrinter::Print(program, __FILE__, __LINE__, google::INFO);
}

TEST_F(ParserTest, Test_Parse_BeginDef2) {  // NOLINT
  ByteTokenStream stream("(begin (define test #t) (define test2 #f))");
  Parser parser(stream);
  const auto program = parser.ParseProgram();
  ASSERT_TRUE(program);
  ast::AstPrinter::Print(program, __FILE__, __LINE__, google::INFO);

  const auto file = fopen("/Users/tazz/Projects/scheme/graph.png", "wb");
  ast::RenderToStdout(program);
  ast::RenderToPng(file, program);
  ASSERT_TRUE(file);
  ASSERT_EQ(fclose(file), 0);
}
}  // namespace scm