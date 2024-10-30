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
  const auto& next = NextToken();
  LOG_IF(FATAL, next.kind != Token::kIdentifier) << "unexpected: " << next << ", expected: " << Token::kIdentifier;
  ASSERT(next.kind == Token::kIdentifier);
  return Symbol::New(next.text);
}

auto Parser::ParseLiteralValue() -> Datum* {
  const auto& next = NextToken();
  switch (next.kind) {
    case Token::kLiteralTrue:
      return Bool::True();
    case Token::kLiteralFalse:
      return Bool::False();
    case Token::kLiteralLong:
      return Long::New(next.AsLong());
    case Token::kLiteralDouble:
      return Double::New(next.AsDouble());
    case Token::kLiteralString:
      return String::New(next.text);
    case Token::kIdentifier:
      return Symbol::New(next.text);
    default:
      LOG(FATAL) << "unexpected: " << NextToken();
      return nullptr;
  }
}

auto Parser::ParseLiteralExpr() -> LiteralExpr* {
  const auto value = ParseLiteralValue();
  ASSERT(value);
  return LiteralExpr::New(value);
}

auto Parser::ParseBeginExpr() -> BeginExpr* {
  ExpectNext(Token::kBeginExpr);
  PushScope();
  const auto begin = BeginExpr::New();
  while (!PeekEq(Token::kRParen)) {
    begin->Append(ParseExpression());
  }
  ASSERT(PeekEq(Token::kRParen));
  PopScope();
  return begin;
}

auto Parser::ParseCallProcExpr() -> CallProcExpr* {
  const auto target = ParseExpression();
  ASSERT(target);
  ExpressionList args;
  while (!PeekEq(Token::kRParen)) {
    const auto arg = ParseExpression();
    ASSERT(arg);
    args.push_back(arg);
  }
  return CallProcExpr::New(target, args);
}

auto Parser::ParseUnaryExpr() -> expr::UnaryExpr* {
  const auto op = NextToken().ToUnaryOp();
  ASSERT(op);
  const auto value = ParseExpression();
  ASSERT(value);
  return expr::UnaryExpr::New((*op), value);
}

auto Parser::ParseBinaryOpExpr() -> BinaryOpExpr* {
  const auto op = NextToken().ToBinaryOp();
  ASSERT(op);
  auto left_expr = ParseExpression();
  auto right_expr = ParseExpression();
  do {
    left_expr = BinaryOpExpr::New((*op), left_expr, right_expr);
    if (PeekEq(Token::kRParen))
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
  LOG_IF(FATAL, !PeekEq(Token::kRParen)) << "unexpected: " << NextToken() << ", expected: " << Token::kRParen;
  return CondExpr::New(test, consequent, alternate);
}

auto Parser::ParseArguments(ArgumentSet& args) -> bool {
  uint64_t num_args = 0;
  while (PeekEq(Token::kIdentifier)) {
    const auto& next = NextToken();
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
  {
    const auto next = PeekToken();
    if (next.IsLiteral() || next.kind == Token::kIdentifier)
      return ParseLiteralExpr();
    else if (next.kind == Token::kQuote)
      return ParseQuotedExpr();
  }

  Expression* expr = nullptr;
  ExpectNext(Token::kLParen);
  const auto next = PeekToken();
  if (next.IsUnaryOp()) {
    expr = ParseUnaryExpr();
  } else if (next.IsBinaryOp()) {
    expr = ParseBinaryOpExpr();
  } else {
    switch (next.kind) {
      // Definitions
      case Token::kLocalDef:
        expr = ParseLocalDef();
        break;
      case Token::kMacroDef:
        expr = ParseMacroDef();
        break;
      // Expressions
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
      case Token::kQuote:
        expr = ParseQuotedExpr();
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

auto Parser::ParseQuotedExpr() -> expr::QuotedExpr* {
  const auto depth = GetDepth();
  DLOG(INFO) << "depth: " << depth;
  ExpectNext(Token::kQuote);
  SkipWhitespace();
  token_len_ = 0;
  do {
    buffer_[token_len_++] = NextChar();
  } while (GetDepth() > depth);
  DLOG(INFO) << "quoted: " << GetBufferedText();
  ASSERT(depth == GetDepth());
  return QuotedExpr::New(GetBufferedText());
}

auto Parser::ParseImportDef() -> expr::ImportDef* {
  ExpectNext(Token::kImportDef);
  const auto symbol = ParseSymbol();
  ASSERT(symbol);
  return ImportDef::New(symbol);
}

auto Parser::ParseMacroDef() -> expr::MacroDef* {
  ExpectNext(Token::kMacroDef);
  const auto symbol = ParseSymbol();
  ASSERT(symbol);

  Expression* body = nullptr;
  if (!PeekEq(Token::kRParen))
    body = ParseExpression();
  return MacroDef::New(symbol, body);
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
  LOG_IF(FATAL, !PeekEq(Token::kRParen)) << "unexpected: " << NextToken() << ", expected: " << Token::kRParen;
  return LocalDef::New(symbol, value);
}

auto Parser::ParseIdentifier(std::string& result) -> bool {
  const auto& next = NextToken();
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
  const auto m = ModuleDef::New(symbol);
  ASSERT(m);
  while (!PeekEq(Token::kRParen)) {
    const auto defn = ParseDefinition();
    ASSERT(defn);
    m->Append(defn);
  }
  ExpectNext(Token::kRParen);
  PopScope();
  return m;
}

auto Parser::ParseDefinition() -> expr::Definition* {
  ExpectNext(Token::kLParen);
  expr::Definition* defn = nullptr;
  const auto& next = PeekToken();
  switch (next.kind) {
    case Token::kLocalDef:
      defn = ParseLocalDef();
      break;
    case Token::kImportDef:
      defn = ParseImportDef();
      break;
    case Token::kMacroDef:
      defn = ParseMacroDef();
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
    const auto& next = PeekToken();
    if (next.IsEndOfStream())
      break;
    program->Append(ParseExpression());
  } while (true);
  return program;
}

auto Parser::PeekToken() -> const Token& {
  if (!peek_.IsInvalid())
    return peek_;
  ASSERT(peek_.IsInvalid());
  return peek_ = NextToken();
}

static inline auto IsValidIdentifierChar(const char c, const bool initial = false) -> bool {
  if (isalpha(c))
    return true;
  else if (isdigit(c) && !initial)
    return true;
  switch (c) {
    case '!':
    case '$':
    case '%':
    case '&':
    case '*':
    case '/':
    case ':':
    case '<':
    case '=':
    case '>':
    case '?':
    case '~':
    case '_':
    case '^':
    case '+':
    case '-':
      return true;
    case '.':
      return !initial;
  }
  return false;
}

static inline auto IsDoubleQuote(const char c) -> bool {
  return c == '\"';
}

static inline auto IsValidStringCharacter(const char c) -> bool {
  return c != EOF && !IsDoubleQuote(c);
}

static inline auto IsValidNumberChar(const char c, const bool whole = true) -> bool {
  return isdigit(c) || (c == '.' && whole);
}

// static inline auto IsValidQuotedChar(const char c) -> bool {
//   switch (c) {
//     case EOF:
//     case ')':
//       return false;
//     default:
//       return true;
//   }
// }

// auto TokenStream::NextQuote() -> const Token& {
//   peek_ = Token{};
//   if (PeekChar() != '(') {
//     const auto next = NextChar();
//     LOG(ERROR) << "unexpected token: " << next;
//     return NextToken(Token::kInvalid, next);
//   }
//   NextChar();

//   while (IsValidQuotedChar(PeekChar())) {
//     buffer_[token_len_++] = NextChar();
//   }

//   if (PeekChar() != ')') {
//     const auto next = NextChar();
//     LOG(ERROR) << "unexpected token: " << next;
//     return NextToken(Token::kInvalid, next);
//   }
//   NextChar();
//   return NextToken(Token::kQuotedExpr, GetBufferedText());
// }

auto Parser::NextToken() -> const Token& {
  if (!peek_.IsInvalid()) {
    next_ = peek_;
    peek_ = Token{};
    return next_;
  }

  const auto next = PeekChar();
  switch (next) {
    case '(':
      Advance();
      return NextToken(Token::kLParen);
    case ')':
      Advance();
      return NextToken(Token::kRParen);
    case '+':
      Advance();
      return NextToken(Token::kPlus);
    case '-':
      Advance();
      return NextToken(Token::kMinus);
    case '*':
      Advance();
      return NextToken(Token::kMultiply);
    case '/':
      Advance();
      return NextToken(Token::kDivide);
    case '%':
      Advance();
      return NextToken(Token::kModulus);
    case '=':
      Advance();
      return NextToken(Token::kEquals);
    case '&':
      Advance();
      return NextToken(Token::kAnd);
    case '|':
      Advance();
      return NextToken(Token::kOr);
    case '!':
      Advance();
      return NextToken(Token::kNot);
    case '#': {
      switch (tolower(PeekChar(1))) {
        case 'f':
          Advance(2);
          return NextToken(Token::kLiteralFalse);
        case 't':
          Advance(2);
          return NextToken(Token::kLiteralTrue);
      }
      Advance();
      return NextToken(Token::kHash, '#');
    }
    case '\n':
    case '\t':
    case '\r':
    case ' ':
      Advance();
      return NextToken();
    case '\'':
      Advance();
      return NextToken(Token::kQuote);
    case ';':
      AdvanceUntil('\n');
      return NextToken();
    case '<': {
      if (PeekChar(1) == '=') {
        Advance(2);
        return NextToken(Token::kLessThanEqual);
      }
      Advance();
      return NextToken(Token::kLessThan);
    }
    case '>': {
      if (PeekChar(1) == '=') {
        Advance(2);
        return NextToken(Token::kGreaterThanEqual);
      }
      Advance();
      return NextToken(Token::kGreaterThan);
    }
    case EOF:
      return NextToken(Token::kEndOfStream);
  }

  if (IsDoubleQuote(next)) {
    Advance();
    token_len_ = 0;
    while (IsValidStringCharacter(PeekChar())) {
      buffer_[token_len_++] = NextChar();
    }
    ASSERT(IsDoubleQuote(PeekChar()));
    Advance();
    return NextToken(Token::kLiteralString, GetBufferedText());
  } else if (isdigit(next)) {
    token_len_ = 0;
    bool whole = true;
    while (IsValidNumberChar(PeekChar())) {
      const auto next = NextChar();
      buffer_[token_len_++] = next;
      if (next == '.')
        whole = false;
    }
    return whole ? NextToken(Token::kLiteralLong, GetBufferedText()) : NextToken(Token::kLiteralDouble, GetBufferedText());
  } else if (IsValidIdentifierChar(next, true)) {
    token_len_ = 0;
    while (IsValidIdentifierChar(PeekChar(), token_len_ == 0)) {
      buffer_[token_len_++] = NextChar();
    }
    const auto ident = GetBufferedText();
    if (ident == "define")
      return NextToken(Token::kLocalDef);
    else if (ident == "defmodule")
      return NextToken(Token::kModuleDef);
    else if (ident == "defmacro")
      return NextToken(Token::kMacroDef);
    else if (ident == "import")
      return NextToken(Token::kImportDef);
    else if (ident == "cons")
      return NextToken(Token::kConsExpr);
    else if (ident == "car")
      return NextToken(Token::kCarExpr);
    else if (ident == "cdr")
      return NextToken(Token::kCdrExpr);
    else if (ident == "begin")
      return NextToken(Token::kBeginExpr);
    else if (ident == "add")
      return NextToken(Token::kPlus);
    else if (ident == "subtract")
      return NextToken(Token::kMinus);
    else if (ident == "multiply")
      return NextToken(Token::kMultiply);
    else if (ident == "divide")
      return NextToken(Token::kDivide);
    else if (ident == "lambda")
      return NextToken(Token::kLambdaExpr);
    else if (ident == "quote")
      return NextToken(Token::kQuote);
    else if (ident == "not")
      return NextToken(Token::kNot);
    else if (ident == "and")
      return NextToken(Token::kAnd);
    else if (ident == "or")
      return NextToken(Token::kOr);
    else if (ident == "throw")
      return NextToken(Token::kThrowExpr);
    else if (ident == "eq?")
      return NextToken(Token::kEquals);
    else if (ident == "set!")
      return NextToken(Token::kSetExpr);
    else if (ident == "cond")
      return NextToken(Token::kCond);
    return NextToken(Token::kIdentifier, ident);
  }

  return NextToken(Token::kInvalid, GetRemaining());
}
}  // namespace scm