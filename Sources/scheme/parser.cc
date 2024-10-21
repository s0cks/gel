#include "scheme/parser.h"

#include <glog/logging.h>

#include "scheme/common.h"
#include "scheme/expression.h"
#include "scheme/instruction.h"

namespace scm {
auto Parser::ParseSymbol() -> Symbol* {
  const auto& next = stream().Next();
  LOG_IF(FATAL, next.kind != Token::kIdentifier) << "unexpected: " << next << ", expected: " << Token::kIdentifier;
  ASSERT(next.kind == Token::kIdentifier);
  return Symbol::New(next.text);
}

auto Parser::ParseLiteralExpr() -> LiteralExpr* {
  const auto& next = stream().Next();
  switch (next.kind) {
    case Token::kLiteralTrue:
      return LiteralExpr::New(Bool::True());
    case Token::kLiteralFalse:
      return LiteralExpr::New(Bool::False());
    case Token::kLiteralLong:
    case Token::kLiteralNumber:
      return LiteralExpr::New(Number::New(next.AsLong()));
    case Token::kLiteralDouble:
    case Token::kLiteralString:
    default:
      LOG(FATAL) << "unexpected: " << stream().Next();
      return nullptr;
  }
}

auto Parser::ParseBeginExpr() -> BeginExpr* {
  const auto begin = BeginExpr::New();
  while (stream().Peek().kind != Token::kRParen) {
    begin->Append(ParseExpression());
  }
  ASSERT(stream().Peek().kind == Token::kRParen);
  return begin;
}

auto Parser::ParseCallProcExpr(std::string name) -> CallProcExpr* {
  ExpressionList args;
  while (stream().Peek().kind != Token::kRParen) {
    const auto arg = ParseExpression();
    ASSERT(arg);
    args.push_back(arg);
  }
  const auto symbol = Symbol::New(name);
  ASSERT(symbol);
  return CallProcExpr::New(symbol, args);
}

static inline auto ToBinaryOp(const Token& rhs) -> BinaryOp {
  switch (rhs.kind) {
    case Token::kPlus:
      return BinaryOp::kAdd;
    case Token::kMinus:
      return BinaryOp::kSubtract;
    case Token::kMultiply:
      return BinaryOp::kMultiply;
    case Token::kDivide:
      return BinaryOp::kDivide;
    case Token::kModulus:
      return BinaryOp::kModulus;
    default:
      LOG(FATAL) << "unexpected: " << rhs;
  }
}

auto Parser::ParseBinaryOpExpr() -> BinaryOpExpr* {
  const auto& next = stream().Next();
  const auto op = ToBinaryOp(next);
  const auto left_expr = ParseExpression();
  const auto right_expr = ParseExpression();
  return BinaryOpExpr::New(op, left_expr, right_expr);
}

static inline auto IsBinaryOp(const Token& rhs) -> bool {
  switch (rhs.kind) {
    case Token::kModulus:
    case Token::kPlus:
    case Token::kMinus:
    case Token::kMultiply:
    case Token::kDivide:
      return true;
    default:
      return false;
  }
}

auto Parser::ParseSymbolExpr() -> SymbolExpr* {
  const auto symbol = ParseSymbol();
  ASSERT(symbol);
  return SymbolExpr::New(symbol);
}

auto Parser::ParseExpression() -> Expression* {
  if (stream().Peek().IsLiteral())
    return ParseLiteralExpr();
  else if (stream().Peek().kind == Token::kIdentifier)
    return ParseSymbolExpr();

  Expression* expr = nullptr;
  ExpectNext(Token::kLParen);
  if (IsBinaryOp(stream().Peek())) {
    expr = ParseBinaryOpExpr();
  } else {
    const auto& next = stream().Next();
    switch (next.kind) {
      case Token::kVariableDef:
        expr = ParseDefineExpr();
        break;
      case Token::kBeginDef:
        expr = ParseBeginExpr();
        break;
      case Token::kIdentifier:
        expr = ParseCallProcExpr(next.text);
        break;
      default:
        LOG(FATAL) << "unexpected: " << next;
        return nullptr;
    }
  }
  ASSERT(expr);
  ExpectNext(Token::kRParen);
  return expr;
}

auto Parser::ParseDefineExpr() -> DefineExpr* {
  const auto symbol = ParseSymbol();
  ASSERT(symbo);
  const auto value = ParseExpression();
  ASSERT(value);
  return DefineExpr::New(symbol, value);
}

auto Parser::ParseProgram() -> Program* {
  const auto program = Program::New();
  do {
    const auto& next = stream().Peek();
    if (next.IsEndOfStream())
      break;
    program->Append(ParseExpression());
  } while (true);
  return program;
}

auto Parse(const uint8_t* data, const uint64_t length) -> Program* {
  ASSERT(data);
  ASSERT(length >= 1);
  ByteTokenStream stream(data, length);
  Parser parser(stream);
  const auto program = parser.ParseProgram();
  ASSERT(program);
  return program;
}
}  // namespace scm