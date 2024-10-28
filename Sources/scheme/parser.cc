#include "scheme/parser.h"

#include <glog/logging.h>

#include "scheme/common.h"
#include "scheme/expression.h"
#include "scheme/instruction.h"
#include "scheme/local.h"
#include "scheme/local_scope.h"
#include "scheme/token.h"

namespace scm {
void Parser::PushScope() {
  const auto old_scope = GetScope();
  ASSERT(old_scope);
  const auto new_scope = LocalScope::New(old_scope);
  ASSERT(new_scope);
  SetScope(new_scope);
}

void Parser::PopScope() {
  const auto old_scope = GetScope();
  ASSERT(old_scope);
  const auto new_scope = old_scope->GetParent();
  ASSERT(new_scope);
  SetScope(new_scope);
}

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
    case Token::kLiteralString:
      return LiteralExpr::New(String::New(next.text));
    case Token::kLiteralDouble:
    default:
      LOG(FATAL) << "unexpected: " << stream().Next();
      return nullptr;
  }
}

auto Parser::ParseBeginExpr() -> BeginExpr* {
  PushScope();
  const auto begin = BeginExpr::New();
  while (stream().Peek().kind != Token::kRParen) {
    begin->Append(ParseExpression());
  }
  ASSERT(stream().Peek().kind == Token::kRParen);
  PopScope();
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
    case Token::kEquals:
      return BinaryOp::kEquals;
    default:
      LOG(FATAL) << "unexpected: " << rhs;
  }
}

auto Parser::ParseBinaryOpExpr() -> BinaryOpExpr* {
  const auto& next = stream().Next();
  const auto op = ToBinaryOp(next);

  auto left_expr = ParseExpression();
  auto right_expr = ParseExpression();
  do {
    left_expr = BinaryOpExpr::New(op, left_expr, right_expr);
    if (stream().Peek().kind == Token::kRParen)
      break;
    right_expr = ParseExpression();
  } while (true);
  ASSERT(left_expr->IsBinaryOp());
  return left_expr->AsBinaryOp();
}

auto Parser::ParseCondExpr() -> CondExpr* {
  const auto test = ParseExpression();
  ASSERT(test);
  const auto consequent = ParseExpression();
  ASSERT(consequent);
  if (PeekEq(Token::kRParen))
    return CondExpr::New(test, consequent);
  const auto alternate = ParseExpression();

  const auto& next = stream().Peek();
  if (next.kind != Token::kRParen) {
    LOG(FATAL) << "unexpected: " << next << "expected: " << Token::kRParen;
    return nullptr;
  }
  return CondExpr::New(test, consequent, alternate);
}

static inline auto IsBinaryOp(const Token& rhs) -> bool {
  switch (rhs.kind) {
    case Token::kModulus:
    case Token::kPlus:
    case Token::kMinus:
    case Token::kMultiply:
    case Token::kDivide:
    case Token::kEquals:
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

auto Parser::ParseArguments(ArgumentSet& args) -> bool {
  uint64_t num_args = 0;
  while (PeekEq(Token::kIdentifier)) {
    const auto& next = stream().Next();
    ASSERT(next.kind == Token::kIdentifier);
    args.insert(Argument(num_args++, next.text));
  }
  return true;
}

auto Parser::ParseSymbolList(SymbolList& symbols) -> bool {
  while (PeekEq(Token::kIdentifier)) {
    const auto next = ParseSymbol();
    ASSERT(next);
    symbols.push_back(next);
  }
  return true;
}

auto Parser::ParseLambdaExpr() -> LambdaExpr* {
  ExpectNext(Token::kLParen);
  ArgumentSet args;
  if (!ParseArguments(args)) {
    LOG(FATAL) << "failed to parse lambda arguments.";
    return nullptr;
  }
  ExpectNext(Token::kRParen);
  return LambdaExpr::New(args, ParseExpression());
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
      case Token::kLocalDef:
        expr = ParseLocalDef();
        break;
      case Token::kBeginExpr:
        expr = ParseBeginExpr();
        break;
      case Token::kIdentifier:
        expr = ParseCallProcExpr(next.text);
        break;
      case Token::kLambdaExpr:
        expr = ParseLambdaExpr();
        break;
      case Token::kCond:
        expr = ParseCondExpr();
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

auto Parser::ParseLocalDef() -> LocalDefExpr* {
  const auto symbol = ParseSymbol();
  ASSERT(symbol);
  if (GetScope()->Has(symbol)) {
    LOG(FATAL) << "cannot redefine symbol: " << symbol;
    return nullptr;
  }
  const auto value = ParseExpression();
  ASSERT(value);
  const auto local = LocalVariable::New(GetScope(), symbol, value->IsConstantExpr() ? value->EvalToConstant() : nullptr);
  ASSERT(local);
  LOG_IF(FATAL, !GetScope()->Add(local)) << "failed to add local: " << local->GetName();
  return LocalDefExpr::New(symbol, value);
}

auto Parser::ParseIdentifier(std::string& result) -> bool {
  const auto& next = stream().Next();
  if (next.kind != Token::kIdentifier) {
    result.clear();
    return Unexpected(Token::kIdentifier, next);
  }
  result = next.text;
  return true;
}

auto Parser::ParseModuleDef() -> expr::ModuleDefExpr* {
  PushScope();
  ExpectNext(Token::kLParen);
  ExpectNext(Token::kModuleDef);
  const auto symbol = ParseSymbol();
  ASSERT(symbol);
  Expression* body = nullptr;
  if (!PeekEq(Token::kRParen)) {
    body = ParseExpression();
    ASSERT(body);
    DLOG(INFO) << "parsed module body: " << body->ToString();
  }
  ExpectNext(Token::kRParen);
  PopScope();
  return ModuleDefExpr::New(symbol, body);
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