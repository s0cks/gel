#include "gel/parser.h"

#include <glog/logging.h>

#include "gel/argument.h"
#include "gel/common.h"
#include "gel/expression.h"
#include "gel/instruction.h"
#include "gel/local.h"
#include "gel/local_scope.h"
#include "gel/module.h"
#include "gel/native_procedure.h"
#include "gel/object.h"
#include "gel/token.h"

namespace gel {
auto Parser::PushScope() -> LocalScope* {
  const auto old_scope = GetScope();
  ASSERT(old_scope);
  const auto new_scope = LocalScope::New(old_scope);
  ASSERT(new_scope);
  SetScope(new_scope);
  return new_scope;
}

void Parser::PopScope() {
  const auto old_scope = GetScope();
  ASSERT(old_scope);
  const auto new_scope = old_scope->GetParent();
  ASSERT(new_scope);
  SetScope(new_scope);
}

auto Parser::ParseLiteralString() -> String* {
  const auto next = ExpectNext(Token::kLiteralString);
  ASSERT(next.kind == Token::kLiteralString);
  if (next.text.empty())
    return String::Empty();
  return String::New(next.text);
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

auto Parser::ParseBinaryExpr() -> BinaryOpExpr* {
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

auto Parser::ParseCondExpr() -> CondExpr* {
  ExpectNext(Token::kCond);
  expr::ClauseList clauses;
  expr::Expression* alt = nullptr;
  do {
    const auto a = ParseExpression();
    ASSERT(a);
    if (PeekEq(Token::kRParen)) {
      alt = a;
      break;
    }
    const auto b = ParseExpression();
    ASSERT(b);
    clauses.push_back(expr::ClauseExpr::New(a, b));
  } while (!PeekEq(Token::kRParen));
  return CondExpr::New(clauses, alt);
}

auto Parser::ParseRxOpExpr() -> expr::RxOpExpr* {
  ExpectNext(Token::kLParen);
  const auto symbol = ParseSymbol();
  ASSERT(symbol);
  expr::ExpressionList args;
  if (!ParseExpressionList(args))
    throw Exception(fmt::format("failed to parse rx-operator `{}` args", (*symbol)));
  ExpectNext(Token::kRParen);
  return expr::RxOpExpr::New(symbol, args);
}

auto Parser::ParseRxOpList(expr::RxOpList& operators) -> bool {
  auto peek = PeekToken();
  while (peek.kind != Token::kRParen && peek.kind != Token::kEndOfStream) {
    const auto oper = ParseRxOpExpr();
    if (!oper)
      return false;
    operators.push_back(oper);
    peek = PeekToken();
  }
  return true;
}

auto Parser::ParseLetRxExpr() -> expr::LetRxExpr* {
  ExpectNext(Token::kLetRxExpr);
  const auto scope = PushScope();
  ASSERT(scope);
  const auto observable = ParseExpression();
  ASSERT(observable);
  expr::RxOpList operators;
  if (PeekEq(Token::kRParen)) {
    PopScope();
    return LetRxExpr::New(scope, observable, operators);
  }

  if (!ParseRxOpList(operators))
    throw Exception("failed to parse rx operators");

  PopScope();
  return LetRxExpr::New(scope, observable, operators);
}

auto Parser::ParseLetExpr() -> expr::LetExpr* {
  ExpectNext(Token::kLetExpr);
  const auto scope = PushScope();
  // parse bindings
  expr::BindingList bindings;
  ExpectNext(Token::kLParen);
  while (!PeekEq(Token::kRParen)) {
    ExpectNext(Token::kLParen);
    const auto symbol = ParseSymbol();
    ASSERT(symbol);
    if (scope->Has(symbol))
      throw Exception(fmt::format("cannot redefine binding for: `{}`", *symbol));
    const auto value = ParseExpression();
    ASSERT(value);
    const auto local = LocalVariable::New(scope, symbol);
    ASSERT(local);
    LOG_IF(FATAL, !scope->Add(local)) << "failed to add " << local << " to scope.";
    bindings.emplace_back(Binding::New(local, value));
    ExpectNext(Token::kRParen);
  }
  ExpectNext(Token::kRParen);
  // parse body
  ExpressionList body;
  if (!ParseExpressionList(body))
    throw Exception("failed to parse let body");
  PopScope();
  return LetExpr::New(scope, bindings, body);
}

auto Parser::ParseArguments(ArgumentSet& args) -> bool {
  ExpectNext(Token::kLBracket);
  uint64_t num_args = 0;
  while (PeekEq(Token::kIdentifier)) {
    const auto& next = NextToken();
    ASSERT(next.kind == Token::kIdentifier);
    args.insert(Argument(num_args++, next.text));
  }
  ExpectNext(Token::kRBracket);
  return true;
}

auto Parser::ParseExpressionList(expr::ExpressionList& expressions) -> bool {
  auto peek = PeekToken();
  while (peek.kind != Token::kRParen && peek.kind != Token::kEndOfStream) {
    const auto expr = ParseExpression();
    if (!expr)
      return false;
    expressions.push_back(expr);
    peek = PeekToken();
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
  const auto value = ParseExpression();
  ASSERT(value);
  return ThrowExpr::New(value);
}

auto Parser::ParseSetExpr() -> SetExpr* {
  ExpectNext(Token::kSetExpr);
  const auto symbol = ParseSymbol();
  ASSERT(symbol);
  const auto scope = GetScope();
  ASSERT(scope);
  LocalVariable* local = nullptr;
  if (!scope->Lookup(symbol, &local)) {
    DLOG(WARNING) << "failed to find local named `" << symbol << "`";
    local = LocalVariable::New(scope, symbol, nullptr);
    ASSERT(local);
    LOG_IF(ERROR, !scope->Add(local)) << "failed to add `" << local << "` to scope:";
    LocalScopePrinter::Print<google::ERROR, false>(scope, __FILE__, __LINE__);
    LOG(FATAL) << "";
  }
  const auto value = ParseExpression();
  ASSERT(value);
  return SetExpr::New(local, value);
}

auto Parser::ParseExpression() -> Expression* {
  {
    auto next = PeekToken();
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
    expr = ParseBinaryExpr();
  } else if (next.IsLiteral()) {
    expr = ParseListExpr();
  } else {
    switch (next.kind) {
      case Token::kDef: {
        expr = ParseLocalDef();
        break;
      }
      // Definitions
      case Token::kMacroDef:
        expr = ParseMacroDef();
        break;
      case Token::kNewExpr:
        expr = ParseNewExpr();
        break;
      // Expressions
      case Token::kBeginExpr:
        expr = ParseBeginExpr();
        break;
      case Token::kFn:
        expr = expr::LiteralExpr::New(ParseLambda(Token::kFn));
        break;
      case Token::kSetExpr:
        expr = ParseSetExpr();
        break;
      case Token::kCond:
        expr = ParseCondExpr();
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
      case Token::kWhenExpr:
        expr = ParseWhenExpr();
        break;
      case Token::kCaseExpr:
        expr = ParseCaseExpr();
        break;
      case Token::kWhileExpr:
        expr = ParseWhileExpr();
        break;
      case Token::kLetRxExpr:
        expr = ParseLetRxExpr();
        break;
      case Token::kCastExpr:
        expr = ParseCastExpr();
        break;
      case Token::kInstanceOfExpr:
        expr = ParseInstanceOfExpr();
        break;
      case Token::kLetExpr:
        expr = ParseLetExpr();
        break;
      default:
        Unexpected(next);
        return nullptr;
    }
  }
  ASSERT(expr);
  ExpectNext(Token::kRParen);
  return expr;
}

auto Parser::ParseQuotedExpr() -> expr::Expression* {
  const auto depth = GetDepth();
  ExpectNext(Token::kQuote);
  SkipWhitespace();
  token_len_ = 0;
  do {
    buffer_[token_len_++] = NextChar();
    if (PeekChar() == ')') {
      if (GetDepth() > depth)
        continue;
      break;
    } else if (IsWhitespaceChar(PeekChar())) {
      if (GetDepth() <= depth)
        break;
    }
  } while (true);
  ASSERT(depth == GetDepth());
  const auto text = GetBufferedText();
  if (text == "()")
    return LiteralExpr::New(Pair::Empty());
  return QuotedExpr::New(GetBufferedText());
}

auto Parser::ParseImportExpr() -> expr::ImportExpr* {
  ExpectNext(Token::kImportExpr);
  const auto symbol = ParseSymbol();
  ASSERT(symbol);
  DVLOG(100) << "importing " << symbol;
  const auto module = Module::Find(symbol->Get());
  LOG_IF(FATAL, !module) << "failed to find Module named `" << symbol->Get() << "`";
  LOG_IF(FATAL, !GetScope()->Add(module->GetScope())) << "failed to import Module `" << symbol->Get() << "` scope.";
  return expr::ImportExpr::New(module);
}

auto Parser::ParseMacroDef() -> expr::MacroDef* {
  ExpectNext(Token::kMacroDef);
  const auto symbol = ParseSymbol();
  ASSERT(symbol);
  ArgumentSet args;
  if (!ParseArguments(args)) {
    LOG(ERROR) << "failed to parse macro args.";
    return nullptr;
  }
  Expression* body = nullptr;
  if (!PeekEq(Token::kRParen))
    body = ParseExpression();
  return MacroDef::New(symbol, args, body);
}

auto Parser::ParseWhenExpr() -> expr::WhenExpr* {
  ExpectNext(Token::kWhenExpr);

  const auto test = ParseExpression();
  ASSERT(test);

  ExpressionList actions;
  if (!ParseExpressionList(actions))
    throw Exception("failed to parse actions.");
  return WhenExpr::New(test, actions);
}

auto Parser::ParseClauseList(expr::ClauseList& clauses) -> bool {
  auto peek = PeekToken();
  while (peek.kind != Token::kRParen && peek.kind != Token::kEndOfStream) {
    ExpectNext(Token::kLParen);
    const auto key = ParseLiteralExpr();
    ASSERT(key);

    expr::ExpressionList actions;
    if (!ParseExpressionList(actions))
      throw Exception("failed to parse actions.");

    clauses.push_back(ClauseExpr::New(key, actions));
    ExpectNext(Token::kRParen);
    peek = PeekToken();
  }
  return true;
}

auto Parser::ParseCaseExpr() -> expr::CaseExpr* {
  ExpectNext(Token::kCaseExpr);
  const auto key = ParseExpression();
  ASSERT(key);
  expr::ClauseList clauses;
  if (!ParseClauseList(clauses))
    throw Exception("failed to parse case clauses.");
  return CaseExpr::New(key, clauses);
}

auto Parser::ParseWhileExpr() -> expr::WhileExpr* {
  ExpectNext(Token::kWhileExpr);
  const auto test = ParseExpression();
  ASSERT(test);
  ExpressionList body;
  if (!ParseExpressionList(body))
    throw Exception("failed to parse loop expression body.");
  return expr::WhileExpr::New(test, body);
}

auto Parser::ParseNewExpr() -> expr::NewExpr* {
  const auto new_expr_token = ExpectNext(Token::kNewExpr);
  const auto symbol = Symbol::New(new_expr_token.text);
  ASSERT(symbol);
  const auto cls = Class::FindClass(symbol);
  LOG_IF(FATAL, !cls) << "failed to find class named: " << symbol;
  expr::ExpressionList args;
  LOG_IF(FATAL, !ParseExpressionList(args)) << "failed to parse new expression args for type: " << cls;
  LOG_IF(FATAL, !PeekEq(Token::kRParen)) << "expected `)`";
  return expr::NewExpr::New(cls, args);
}

auto Parser::ParseLocalDef() -> LocalDef* {
  ExpectNext(Token::kDef);
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
  return LocalDef::New(local, value);
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

auto Parser::ParseDefinition() -> expr::Definition* {
  ExpectNext(Token::kLParen);
  expr::Definition* defn = nullptr;
  const auto& next = PeekToken();
  switch (next.kind) {
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
    case '.': {
      Advance();
      if (PeekChar() == '.') {
        Advance();
        if (PeekChar() == '.') {
          Advance();
          return NextToken(Token::kRange);
        }
        return NextToken(Token::kInvalid);
      }
      return NextToken(Token::kDot);
    }
    case '+':
      Advance();
      return NextToken(Token::kAdd);
    case '-':
      Advance();
      return NextToken(Token::kSubtract);
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
      return NextToken(Token::kBinaryAnd);
    case '|':
      Advance();
      return NextToken(Token::kBinaryOr);
    case '!':
      Advance();
      return NextToken(Token::kNot);
    case '[':
      Advance();
      return NextToken(Token::kLBracket);
    case ']':
      Advance();
      return NextToken(Token::kRBracket);
    case '#': {
      switch (tolower(PeekChar(1))) {
        case 'f':
          Advance(2);
          return NextToken(Token::kLiteralFalse);
        case 't':
          Advance(2);
          return NextToken(Token::kLiteralTrue);
      }
      if (IsValidIdentifierChar(PeekChar(1))) {
        Advance();
        token_len_ = 0;
        while (IsValidIdentifierChar(PeekChar(), token_len_ == 0) && PeekChar() != '?') {
          buffer_[token_len_++] = NextChar();
        }
        LOG_IF(FATAL, PeekChar() != '?') << "expected `?` not: " << NextToken();
        Advance();
        return NextToken(Token::kInstanceOfExpr, GetBufferedText());
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
    case ':':
      if (PeekChar(1) == '-' && PeekChar(2) == '>') {
        Advance(3);
        token_len_ = 0;
        while (IsValidIdentifierChar(PeekChar(), token_len_ == 0)) {
          buffer_[token_len_++] = NextChar();
        }
        return NextToken(Token::kCastExpr, GetBufferedText());
      }
      break;
    case 'n': {
      if (PeekChar(1) == 'e' && PeekChar(2) == 'w') {
        Advance(3);
        LOG_IF(FATAL, PeekChar() != ':') << "expected char `:` not: " << NextToken();
        NextChar();
        token_len_ = 0;
        while (IsValidIdentifierChar(PeekChar(), token_len_ == 0)) {
          buffer_[token_len_++] = NextChar();
        }
        return NextToken(Token::kNewExpr, GetBufferedText());
      }
      break;
    }
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
    if (ident == "ns")
      return NextToken(Token::kDefNamespace);
    else if (ident == "def")
      return NextToken(Token::kDef);
    else if (ident == "defmacro")
      return NextToken(Token::kMacroDef);
    else if (ident == "import")
      return NextToken(Token::kImportExpr);
    else if (ident == "cons")
      return NextToken(Token::kCons);
    else if (ident == "car")
      return NextToken(Token::kCar);
    else if (ident == "cdr")
      return NextToken(Token::kCdr);
    else if (ident == "begin")
      return NextToken(Token::kBeginExpr);
    else if (ident == "add")
      return NextToken(Token::kAdd);
    else if (ident == "subtract")
      return NextToken(Token::kSubtract);
    else if (ident == "multiply")
      return NextToken(Token::kMultiply);
    else if (ident == "divide")
      return NextToken(Token::kDivide);
    else if (ident == "fn")
      return NextToken(Token::kFn);
    else if (ident == "quote")
      return NextToken(Token::kQuote);
    else if (ident == "not")
      return NextToken(Token::kNot);
    else if (ident == "and")
      return NextToken(Token::kBinaryAnd);
    else if (ident == "or")
      return NextToken(Token::kBinaryOr);
    else if (ident == "throw")
      return NextToken(Token::kThrowExpr);
    else if (ident == "eq?")
      return NextToken(Token::kEquals);
    else if (ident == "instanceof?")
      return NextToken(Token::kInstanceOf);
    else if (ident == "nonnull?")
      return NextToken(Token::kNonnull);
    else if (ident == "null?")
      return NextToken(Token::kNull);
    else if (ident == "set!")
      return NextToken(Token::kSetExpr);
    else if (ident == "cond")
      return NextToken(Token::kCond);
    else if (ident == "when")
      return NextToken(Token::kWhenExpr);
    else if (ident == "case")
      return NextToken(Token::kCaseExpr);
    else if (ident == "while")
      return NextToken(Token::kWhileExpr);
    else if (ident == "defn")
      return NextToken(Token::kDefn);
    else if (ident == "let")
      return NextToken(Token::kLetExpr);
    else if (ident == "let:rx")
      return NextToken(Token::kLetRxExpr);
    else if (ident == "defnative")
      return NextToken(Token::kDefNative);
    return NextToken(Token::kIdentifier, ident);
  }

  return NextToken(Token::kInvalid, GetRemaining());
}

auto Parser::ParseCastExpr() -> expr::CastExpr* {
  const auto token = ExpectNext(Token::kCastExpr);
  ASSERT(!token.text.empty());
  const auto symbol = Symbol::New(token.text);
  const auto cls = Class::FindClass(symbol);
  if (!cls) {
    LOG(FATAL) << "cannot create cast, failed to find type: " << symbol;
    return nullptr;
  }
  return expr::CastExpr::New(cls, ParseExpression());
}

auto Parser::ParseInstanceOfExpr() -> expr::InstanceOfExpr* {
  const auto token = ExpectNext(Token::kInstanceOfExpr);
  ASSERT(!token.text.empty());
  const auto symbol = Symbol::New(token.text);
  const auto cls = Class::FindClass(symbol);
  if (!cls) {
    LOG(FATAL) << "cannot create cast, failed to find type: " << symbol;
    return nullptr;
  }
  return expr::InstanceOfExpr::New(cls, ParseExpression());
}

auto Parser::ParseLocalVariable(LocalVariable** local, expr::Expression** value) -> bool {
  ExpectNext(Token::kDef);
  const auto symbol = ParseSymbol();
  ASSERT(symbol);
  (*value) = ParseExpression();
  ASSERT(value);
  if ((*value)->IsConstantExpr()) {
    const auto cvalue = (*value)->EvalToConstant();
    DLOG(INFO) << "cvalue: " << cvalue;
    (*local) = LocalVariable::New(GetScope(), symbol, cvalue);
    (*value) = nullptr;
  } else {
    (*local) = LocalVariable::New(GetScope(), symbol, nullptr);
  }
  ASSERT(local);
  // TODO: store value if not constant
  if (!GetScope()->Add((*local)))
    throw Exception(fmt::format("failed to add LocalVariable: {}", (*local)->GetName()));
  return true;
}

auto Parser::ParseNamespace() -> Namespace* {
  ExpectNext(Token::kDefNamespace);
  const auto name = ParseSymbol();
  ASSERT(name);
  const auto parent_scope = GetScope();
  const auto scope = LocalScope::New();
  ASSERT(scope);
  const auto ns = Namespace::New(String::New(name->Get()), scope);
  ASSERT(ns);
  SetNamespace(ns);
  if (PeekEq(Token::kLiteralString)) {
    const auto docstring = ParseLiteralString();
    ASSERT(docstring);
    ns->SetDocs(docstring);
  }
  while (!PeekEq(Token::kRParen)) {
    ExpectNext(Token::kLParen);
    const auto next = PeekToken();
    switch (next.kind) {
      case Token::kDefn: {
        const auto lambda = ParseLambda(Token::kDefn);
        ASSERT(lambda && lambda->HasName());
        LOG_IF(FATAL, !scope->Add(lambda->GetName(), lambda)) << "failed to add " << lambda << " to scope.";
        break;
      }
      case Token::kDefNative: {
        ExpectNext(Token::kDefNative);
        const auto symbol = GetNamespace()->Prefix(ParseSymbol());
        ASSERT(symbol);
        const auto native = NativeProcedure::Find(symbol);
        LOG_IF(FATAL, !native) << "failed to find native named: " << symbol;
        LOG_IF(FATAL, !scope->Add(symbol, native)) << "failed to add " << native << " to scope.";
        // arguments
        ArgumentSet args;
        if (!ParseArguments(args))
          throw Exception("failed to parse ArgumentSet");
        native->SetArgs(args);
        // docstring
        String* docs = nullptr;
        if (PeekEq(Token::kLiteralString)) {
          docs = ParseLiteralString();
          ASSERT(docs);
        }
        native->SetDocs(docs);
        break;
      }
      default:
        Unexpected(next);
        return nullptr;
    }
    ExpectNext(Token::kRParen);
  }
  ClearNamespace();
  SetScope(parent_scope);
  return ns;
}

auto Parser::ParseLambda(const Token::Kind kind) -> Lambda* {
  ExpectNext(kind);
  // name
  Symbol* name = nullptr;
  if (PeekEq(Token::kIdentifier)) {
    name = ParseSymbol();
    ASSERT(name);
    if (InNamespace())
      name = GetNamespace()->Prefix(name);
    ASSERT(name);
  }
  // arguments
  ArgumentSet args;
  if (!ParseArguments(args))
    throw Exception("failed to parse ArgumentSet");
  // docstring
  String* docs = nullptr;
  if (PeekEq(Token::kLiteralString)) {
    docs = ParseLiteralString();
    ASSERT(docs);
  }
  // body
  ExpressionList body;
  PushScope();
  if (!ParseExpressionList(body)) {
    LOG(FATAL) << "failed to parse lambda body.";
    return nullptr;
  }
  PopScope();
  const auto lambda = Lambda::New(args, body);
  ASSERT(lambda);
  if (name)
    lambda->SetName(name);
  DLOG_IF(FATAL, name && GetScope()->Has(name)) << "cannot redefine: " << name;
  if (docs) {
    if (body.empty()) {
      body.push_back(expr::LiteralExpr::New(docs));
    } else {
      lambda->SetDocstring(docs);
    }
  }
  return lambda;
}

static inline auto IsLiteralLong(Expression* expr) -> bool {
  if (!expr || !expr->IsLiteralExpr())
    return false;
  const auto literal = expr->AsLiteralExpr();
  ASSERT(literal);
  return literal->HasValue() && literal->GetValue()->IsLong();
}

auto Parser::ParseListExpr() -> expr::Expression* {
  const auto first = ParseExpression();
  if (PeekEq(Token::kRange)) {
    NextToken();
    LOG_IF(FATAL, !IsLiteralLong(first)) << "expected " << first << " to be a literal Long.";
    const auto end = ParseExpression();
    LOG_IF(FATAL, !IsLiteralLong(end)) << "expected " << end << " to be a literal Long.";
    const auto from = first->AsLiteralExpr()->GetValue()->AsLong()->Get();
    const auto to = end->AsLiteralExpr()->GetValue()->AsLong()->Get();
    return expr::LiteralExpr::New(gel::ListFromRange(from, to));
  }
  const auto list = expr::ListExpr::New();
  list->Append(first);
  while (!PeekEq(Token::kRParen)) {
    list->Append(ParseExpression());
  }
  return list;
}

auto Parser::ParseModule(const std::string& name) -> Module* {
  const auto scope = PushScope();
  ASSERT(scope);
  const auto new_module = Module::New(String::New(name), scope);
  ASSERT(new_module);
  while (!PeekEq(Token::kEndOfStream)) {
    ExpectNext(Token::kLParen);
    const auto next = PeekToken();
    switch (next.kind) {
      case Token::kDefNamespace: {
        const auto ns = ParseNamespace();
        ASSERT(ns);
        LOG_IF(FATAL, !scope->Add(Symbol::New(ns->GetName()->Get()), ns)) << "failed to add " << ns << " to scope.";
        LOG_IF(FATAL, !scope->Add(ns->GetScope())) << "failed to add " << ns << " to scope.";
        break;
      }
      default:
        Unexpected(next);
        return nullptr;
    }
    ExpectNext(Token::kRParen);
  }
  PopScope();
  return new_module;
}

auto Parser::ParseScript() -> Script* {
  const auto scope = PushScope();
  ASSERT(scope);
  const auto script = Script::New(scope);
  ASSERT(script);
  while (!PeekEq(Token::kEndOfStream)) {
    const auto& peek = PeekToken();
    if (peek.IsLiteral() || peek.IsIdentifier()) {
      script->Append(ParseLiteralExpr());
    } else if (peek.IsQuote()) {
      script->Append(ParseQuotedExpr());
    }

    Expression* expr = nullptr;
    ExpectNext(Token::kLParen);
    const auto next = PeekToken();
    if (next.IsUnaryOp()) {
      expr = ParseUnaryExpr();
    } else if (next.IsBinaryOp()) {
      expr = ParseBinaryExpr();
    } else if (next.IsLiteral()) {
      expr = ParseListExpr();
    } else {
      switch (next.kind) {
        case Token::kDefNamespace: {
          const auto ns = ParseNamespace();
          ASSERT(ns);
          script->Append(ns);
          LOG_IF(FATAL, !scope->Add(ns->GetScope())) << "failed to add " << ns << " to scope.";
          break;
        }
        // Definitions
        case Token::kDef: {
          LocalVariable* local = nullptr;
          expr::Expression* value = nullptr;
          if (!ParseLocalVariable(&local, &value)) {
            LOG(FATAL) << "failed to parse local variable";
            break;
          }
          ASSERT(local);
          if (value)
            expr = expr::SetExpr::New(local, value);
          break;
        }
        case Token::kDefn: {
          const auto lambda = ParseLambda(Token::kDefn);
          ASSERT(lambda && lambda->HasName());
          const auto local = LocalVariable::New(GetScope(), lambda->GetName(), lambda);
          ASSERT(local);
          scope->Add(local);
          script->Append(lambda);
          break;
        }
        case Token::kMacroDef:
          expr = ParseMacroDef();
          break;
        // Expressions
        case Token::kBeginExpr:
          expr = ParseBeginExpr();
          break;
        case Token::kFn:
          expr = expr::LiteralExpr::New(ParseLambda(Token::kFn));
          break;
        case Token::kSetExpr:
          expr = ParseSetExpr();
          break;
        case Token::kCond:
          expr = ParseCondExpr();
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
        case Token::kWhenExpr:
          expr = ParseWhenExpr();
          break;
        case Token::kCaseExpr:
          expr = ParseCaseExpr();
          break;
        case Token::kWhileExpr:
          expr = ParseWhileExpr();
          break;
        case Token::kLetExpr:
          expr = ParseLetExpr();
          break;
        case Token::kLetRxExpr:
          expr = ParseLetRxExpr();
          break;
        case Token::kCastExpr:
          expr = ParseCastExpr();
          break;
        case Token::kInstanceOfExpr:
          expr = ParseInstanceOfExpr();
          break;
        case Token::kImportExpr:
          expr = ParseImportExpr();
          break;
        default:
          Unexpected(next);
          return nullptr;
      }
    }
    ExpectNext(Token::kRParen);
    if (expr) {
      script->Append(expr);
      DVLOG(100) << "parsed: " << expr->ToString();
    }
  }
  PopScope();
  return script;
}
}  // namespace gel