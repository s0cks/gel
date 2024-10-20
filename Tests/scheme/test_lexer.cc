#include <gtest/gtest.h>

#include "gtest/gtest.h"
#include "scheme/lexer.h"

namespace scm {
using namespace ::testing;

class LexerTest : public Test {};

TEST_F(LexerTest, Test_Parse_EndOfStream) {  // NOLINT
  ByteTokenStream stream("");
  const auto& next = stream.Next();
  ASSERT_TRUE(next.IsEndOfStream());
}

static inline auto IsKind(const Token::Kind expected, const Token& actual) -> AssertionResult {
  if (actual.kind != expected)
    return AssertionFailure() << "expected " << actual << " to be: " << expected;
  return AssertionSuccess();
}

static inline auto HasText(const std::string& expected, const Token& actual) -> AssertionResult {
  if (actual.text != expected)
    return AssertionFailure() << "expected " << actual << " to be: " << expected;
  return AssertionSuccess();
}

static inline auto HasPos(const Position& expected, const Token& actual) -> AssertionResult {
  if (actual.pos != expected)
    return AssertionFailure() << "expected " << actual << " to be: " << expected;
  return AssertionSuccess();
}

static inline auto HasPos(const uint64_t column, const uint64_t row, const Token& actual) -> AssertionResult {
  return HasPos(Position{.row = row, .column = column}, actual);
}

static inline auto IsNext(const Token::Kind expected_kind, TokenStream& stream) -> AssertionResult {
  const auto& next = stream.Next();
  {
    const auto result = IsKind(expected_kind, next);
    if (!result)
      return result;
  }
  return AssertionSuccess();
}

static inline auto IsNext(const Token::Kind expected_kind, const std::string& expected_text, TokenStream& stream)
    -> AssertionResult {
  const auto& next = stream.Next();
  {
    const auto result = IsKind(expected_kind, next);
    if (!result)
      return result;
  }
  {
    const auto result = HasText(expected_text, next);
    if (!result)
      return result;
  }
  return AssertionSuccess();
}

TEST_F(LexerTest, Test_Next_LiteralLong) {  // NOLINT
  ByteTokenStream stream("128737819");
  ASSERT_TRUE(IsNext(Token::kLiteralLong, "128737819", stream));
}

TEST_F(LexerTest, Test_Next_LiteralDouble) {  // NOLINT
  ByteTokenStream stream("128.737819");
  ASSERT_TRUE(IsNext(Token::kLiteralDouble, "128.737819", stream));
}

TEST_F(LexerTest, Test_Next_LiteralTrueLowercase) {  // NOLINT
  ByteTokenStream stream("#t");
  ASSERT_TRUE(IsNext(Token::kLiteralTrue, stream));
}

TEST_F(LexerTest, Test_Next_LiteralTrueUppercase) {  // NOLINT
  ByteTokenStream stream("#T");
  ASSERT_TRUE(IsNext(Token::kLiteralTrue, stream));
}

TEST_F(LexerTest, Test_Next_LiteralFalseLowercase) {  // NOLINT
  ByteTokenStream stream("#f");
  ASSERT_TRUE(IsNext(Token::kLiteralFalse, stream));
}

TEST_F(LexerTest, Test_Next_LiteralFalseUppercase) {  // NOLINT
  ByteTokenStream stream("#F");
  ASSERT_TRUE(IsNext(Token::kLiteralFalse, stream));
}

TEST_F(LexerTest, Test_Next_LParen) {  // NOLINT
  ByteTokenStream stream("(");
  ASSERT_TRUE(IsNext(Token::kLParen, stream));
}

TEST_F(LexerTest, Test_Next_RParen) {  // NOLINT
  ByteTokenStream stream(")");
  ASSERT_TRUE(IsNext(Token::kRParen, stream));
}

TEST_F(LexerTest, Test_Next_Define) {  // NOLINT
  ByteTokenStream stream("define");
  ASSERT_TRUE(IsNext(Token::kVariableDef, stream));
}

TEST_F(LexerTest, Test_Next_Begin) {  // NOLINT
  ByteTokenStream stream("begin");
  ASSERT_TRUE(IsNext(Token::kBeginDef, stream));
}

TEST_F(LexerTest, Test_Next_Plus_Shorthand) {  // NOLINT
  ByteTokenStream stream("+");
  ASSERT_TRUE(IsNext(Token::kPlus, stream));
}

TEST_F(LexerTest, Test_Next_Plus) {  // NOLINT
  ByteTokenStream stream("add");
  ASSERT_TRUE(IsNext(Token::kPlus, stream));
}

TEST_F(LexerTest, Test_Next_Minus_Shorthand) {  // NOLINT
  ByteTokenStream stream("-");
  ASSERT_TRUE(IsNext(Token::kMinus, stream));
}

TEST_F(LexerTest, Test_Next_Minus) {  // NOLINT
  ByteTokenStream stream("subtract");
  ASSERT_TRUE(IsNext(Token::kMinus, stream));
}

TEST_F(LexerTest, Test_Next_Multiply_Shorthand) {  // NOLINT
  ByteTokenStream stream("*");
  ASSERT_TRUE(IsNext(Token::kMultiply, stream));
}

TEST_F(LexerTest, Test_Next_Multiply) {  // NOLINT
  ByteTokenStream stream("multiply");
  ASSERT_TRUE(IsNext(Token::kMultiply, stream));
}

TEST_F(LexerTest, Test_Next_Divide_Shorthand) {  // NOLINT
  ByteTokenStream stream("/");
  ASSERT_TRUE(IsNext(Token::kDivide, stream));
}

TEST_F(LexerTest, Test_Next_Divide) {  // NOLINT
  ByteTokenStream stream("divide");
  ASSERT_TRUE(IsNext(Token::kDivide, stream));
}

TEST_F(LexerTest, Test_Next_Modulus_Shorthand) {  // NOLINT
  ByteTokenStream stream("%");
  ASSERT_TRUE(IsNext(Token::kModulus, stream));
}

TEST_F(LexerTest, Test_Next_Identifier) {  // NOLINT
  ByteTokenStream stream("print");
  ASSERT_TRUE(IsNext(Token::kIdentifier, "print", stream));
}

TEST_F(LexerTest, Test_Next_Hash) {  // NOLINT
  ByteTokenStream stream("#");
  ASSERT_TRUE(IsNext(Token::kHash, stream));
}

TEST_F(LexerTest, Test_Quote_Shorthand) {  // NOLINT
  ByteTokenStream stream("'");
  ASSERT_TRUE(IsNext(Token::kQuote, stream));
}

TEST_F(LexerTest, Test_Quote) {  // NOLINT
  ByteTokenStream stream("quote");
  ASSERT_TRUE(IsNext(Token::kQuote, stream));
}

TEST_F(LexerTest, Test_Literal_String) {  // NOLINT
  ByteTokenStream stream("\"Hello World\"");
  ASSERT_TRUE(IsNext(Token::kLiteralString, "Hello World", stream));
}

TEST_F(LexerTest, Test_Lambda) {  // NOLINT
  ByteTokenStream stream("lambda");
  ASSERT_TRUE(IsNext(Token::kLambdaDef, stream));
}
}  // namespace scm