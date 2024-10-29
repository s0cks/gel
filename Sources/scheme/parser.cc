#include "scheme/parser.h"

#include <glog/logging.h>

#include "scheme/common.h"
#include "scheme/expression.h"
#include "scheme/instruction.h"
#include "scheme/local.h"
#include "scheme/local_scope.h"
#include "scheme/token.h"

namespace scm {
static inline auto IsValidBinaryOp(const Token& rhs) -> bool {
  switch (rhs.kind) {
    case Token::kPlus:
    case Token::kMinus:
    case Token::kMultiply:
    case Token::kDivide:
    case Token::kModulus:
    case Token::kAnd:
    case Token::kOr:
    case Token::kEquals:
      return true;
    default:
      return false;
  }
}

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
      return LiteralExpr::New(Long::New(next.AsLong()));
    case Token::kLiteralDouble:
      return LiteralExpr::New(Double::New(next.AsDouble()));
    case Token::kLiteralString:
      return LiteralExpr::New(String::New(next.text));
    default:
      LOG(FATAL) << "unexpected: " << stream().Next();
      return nullptr;
  }
}

auto Parser::ParseBeginExpr() -> BeginExpr* {
  ExpectNext(Token::kBeginExpr);
  PushScope();
  const auto begin = BeginExpr::New();
  while (stream().Peek().kind != Token::kRParen) {
    begin->Append(ParseExpression());
  }
  ASSERT(stream().Peek().kind == Token::kRParen);
  PopScope();
  return begin;
}

auto Parser::ParseCallProcExpr() -> CallProcExpr* {
  const auto target = ParseExpression();
  ASSERT(target);
  ExpressionList args;
  while (stream().Peek().kind != Token::kRParen) {
    const auto arg = ParseExpression();
    ASSERT(arg);
    args.push_back(arg);
  }
  return CallProcExpr::New(target, args);
}

static inline auto ToBinaryOp(const Token& rhs) -> BinaryOp {
  ASSERT(IsValidBinaryOp(rhs));
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
    case Token::kAnd:
      return BinaryOp::kBinaryAnd;
    case Token::kOr:
      return BinaryOp::kBinaryOr;
    default:
      LOG(FATAL) << "unexpected: " << rhs;
  }
}

static inline auto ToUnaryOp(const Token& rhs) -> expr::UnaryOp {
  switch (rhs.kind) {
    case Token::kNot:
      return expr::UnaryOp::kNot;
    case Token::kCarExpr:
      return expr::UnaryOp::kCar;
    case Token::kCdrExpr:
      return expr::UnaryOp::kCdr;
    default:
      LOG(FATAL) << "invalid UnaryOp: " << rhs;
  }
}

static inline auto IsValidUnaryOp(const Token& rhs) -> bool {
  switch (rhs.kind) {
    case Token::kNot:
    case Token::kCarExpr:
    case Token::kCdrExpr:
      return true;
    default:
      return false;
  }
}

auto Parser::ParseUnaryExpr() -> expr::UnaryExpr* {
  const auto op = ToUnaryOp(stream().Next());
  const auto value = ParseExpression();
  ASSERT(value);
  return expr::UnaryExpr::New(op, value);
}

auto Parser::ParseBinaryOpExpr() -> BinaryOpExpr* {
  const auto op = ToBinaryOp(stream().Next());
  auto left_expr = ParseExpression();
  auto right_expr = ParseExpression();
  do {
    left_expr = BinaryOpExpr::New(op, left_expr, right_expr);
    if (stream().Peek().kind == Token::kRParen)
      break;
    right_expr = ParseExpression();
  } while (true);
  ASSERT(left_expr->IsBinaryOpExpr());
  return left_expr->AsBinaryOpExpr();
}

auto Parser::ParseConsExpr() -> ConsExpr* {
  ExpectNext(Token::kConsExpr);
  const auto car = ParseExpression();
  ASSERT(car);
  const auto cdr = ParseExpression();
  ASSERT(cdr);
  return ConsExpr::New(car, cdr);
}

auto Parser::ParseCondExpr() -> CondExpr* {
  ExpectNext(Token::kCond);
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

auto Parser::ParseThrowExpr() -> ThrowExpr* {
  ExpectNext(Token::kThrowExpr);
  const auto value = ParseLiteralExpr();
  ASSERT(value);
  return ThrowExpr::New(value);
}

auto Parser::ParseLambdaExpr() -> LambdaExpr* {
  ExpectNext(Token::kLambdaExpr);
  ExpectNext(Token::kLParen);
  ArgumentSet args;
  if (!ParseArguments(args)) {
    LOG(FATAL) << "failed to parse lambda arguments.";
    return nullptr;
  }
  ExpectNext(Token::kRParen);
  return LambdaExpr::New(args, ParseExpression());
}

auto Parser::ParseSetExpr() -> SetExpr* {
  ExpectNext(Token::kSetExpr);
  const auto symbol = ParseSymbol();
  ASSERT(symbol);
  const auto value = ParseExpression();
  ASSERT(value);
  return SetExpr::New(symbol, value);
}

auto Parser::ParseExpression() -> Expression* {
  if (stream().Peek().IsLiteral())
    return ParseLiteralExpr();
  else if (stream().Peek().kind == Token::kIdentifier)
    return ParseSymbolExpr();

  Expression* expr = nullptr;
  ExpectNext(Token::kLParen);
  if (IsValidUnaryOp(stream().Peek())) {
    expr = ParseUnaryExpr();
  } else if (IsValidBinaryOp(stream().Peek())) {
    expr = ParseBinaryOpExpr();
  } else {
    const auto& next = stream().Peek();
    switch (next.kind) {
      case Token::kLocalDef:
        expr = ParseLocalDef();
        break;
      case Token::kBeginExpr:
        expr = ParseBeginExpr();
        break;
      case Token::kLambdaExpr:
        expr = ParseLambdaExpr();
        break;
      case Token::kSetExpr:
        expr = ParseSetExpr();
        break;
      case Token::kCond:
        expr = ParseCondExpr();
        break;
      case Token::kConsExpr:
        expr = ParseConsExpr();
        break;
      case Token::kThrowExpr:
        expr = ParseThrowExpr();
        break;
      case Token::kLParen:
      case Token::kIdentifier:
        expr = ParseCallProcExpr();
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

auto Parser::ParseImportDef() -> expr::ImportDef* {
  ExpectNext(Token::kImportDef);
  const auto symbol = ParseSymbol();
  ASSERT(symbol);
  return ImportDef::New(symbol);
}

auto Parser::ParseLocalDef() -> LocalDef* {
  ExpectNext(Token::kLocalDef);
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
  return LocalDef::New(symbol, value);
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

auto Parser::ParseModuleDef() -> expr::ModuleDef* {
  PushScope();
  ExpectNext(Token::kLParen);
  ExpectNext(Token::kModuleDef);
  const auto symbol = ParseSymbol();
  ASSERT(symbol);
  const auto module = ModuleDef::New(symbol);
  ASSERT(module);
  while (PeekEq(Token::kLParen)) {
    module->Append(ParseDefinition());
  }
  ExpectNext(Token::kRParen);
  PopScope();
  return module;
}

auto Parser::ParseDefinition() -> expr::Definition* {
  ExpectNext(Token::kLParen);
  expr::Definition* defn = nullptr;
  const auto& next = stream().Peek();
  switch (next.kind) {
    case Token::kLocalDef:
      defn = ParseLocalDef();
      break;
    case Token::kImportDef:
      defn = ParseImportDef();
      break;
    default:
      LOG(FATAL) << "unexpected: " << next << ", expected definition.";
      return nullptr;
  }
  ExpectNext(Token::kRParen);
  ASSERT(defn);
  return defn;
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