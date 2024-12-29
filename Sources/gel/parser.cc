#include "gel/parser.h"

#include <glog/logging.h>

#include <utility>

#include "gel/argument.h"
#include "gel/common.h"
#include "gel/expression.h"
#include "gel/instruction.h"
#include "gel/local.h"
#include "gel/local_scope.h"
#include "gel/macro.h"
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
  const auto& next = NextToken();  // TODO: fix the weird logic where kNewExpr check is needed
  LOG_IF(FATAL, (next.kind != Token::kIdentifier && next.kind != Token::kNewExpr))
      << "unexpected: " << next << ", expected: " << Token::kIdentifier;
  ASSERT(next.kind == Token::kIdentifier || next.kind == Token::kNewExpr);
  if (InNamespace())
    return GetNamespace()->CreateSymbol(next.text);
  return Symbol::New(next.text);
}

auto Parser::ParseLiteralLambda(const Token::Kind kind) -> expr::LiteralExpr* {
  return expr::LiteralExpr::New(ParseLambda(kind));
}

auto Parser::ParseMap() -> expr::Expression* {
  ExpectNext(Token::kLBrace);
  expr::NewMapExpr::EntryList data{};
  while (!PeekEq(Token::kRBrace)) {
    const auto key = ParseSymbol();
    ASSERT(key);
    const auto value = ParseExpression();
    ASSERT(value);
    if (PeekEq(Token::kComma))
      NextToken();
    data.emplace_back(key, value);
  }
  ExpectNext(Token::kRBrace);
  return expr::NewMapExpr::New(data);
}

auto Parser::ParseLiteralBool() -> Bool* {
  const auto& next = NextToken();
  switch (next.kind) {
    case Token::kLiteralTrue:
      return Bool::True();
    case Token::kLiteralFalse:
      return Bool::False();
    default:
      Unexpected(next, Token::AnyBool());
      return nullptr;
  }
}

auto Parser::ParseLiteralNumber() -> Number* {
  const auto& next = NextToken();
  switch (next.kind) {
    case Token::kLiteralLong:
      return Long::New(next.AsLong());
    case Token::kLiteralDouble:
      return Double::New(next.AsDouble());
    default:
      Unexpected(next, Token::AnyNumber());
      return nullptr;
  }
}

auto Parser::ParseLiteralValue() -> Object* {
  switch (PeekKind()) {
    case Token::kLiteralFalse:
    case Token::kLiteralTrue:
      return ParseLiteralBool();
    case Token::kLiteralLong:
    case Token::kLiteralDouble:
      return ParseLiteralNumber();

    case Token::kLiteralString:
      return String::New(NextToken().text);
    case Token::kIdentifier:
      return Symbol::New(NextToken().text);
    default:
      LOG(FATAL) << "unexpected: " << NextToken();
      return nullptr;
  }
}

auto Parser::ParseLiteralExpr() -> expr::Expression* {
  if (PeekEq(Token::kFn) || PeekEq(Token::kDispatch)) {
    return ParseLiteralLambda(PeekKind());
  } else if (PeekEq(Token::kLBrace)) {
    return ParseMap();
  }
  const auto value = ParseLiteralValue();
  ASSERT(value);
  return LiteralExpr::New(value);
}

auto Parser::ParseBeginExpr() -> BeginExpr* {
  ExpectNext(Token::kBeginExpr);
  PushScope();
  const auto begin = BeginExpr::New();
  while (!PeekEq(Token::kRParen)) {
    const auto next = ParseExpression();
    if (next)
      begin->Append(next);
  }
  ASSERT(PeekEq(Token::kRParen));
  PopScope();
  return begin;
}

static inline auto IsClassReference(expr::Expression* expr) -> bool {
  if (IsLiteralSymbol(expr)) {
    const auto symbol = expr->AsLiteralExpr()->GetValue()->AsSymbol();
    ASSERT(symbol);
    return Class::FindClass(symbol) != nullptr;
  }
  return false;
}

auto Parser::ParseCallExpr() -> expr::Expression* {
  Expression* target = nullptr;
  if (PeekEq(Token::kIdentifier)) {
    const auto symbol = ParseSymbol();
    ASSERT(symbol);
    if (symbol->HasSymbolType()) {
      const auto cls = Class::FindClass(symbol->GetSymbolType());
      if (cls) {
        ASSERT(cls);
        const auto func = cls->GetFunction(symbol);
        if (!func)
          LOG(FATAL) << "cannot find function: " << symbol;
        ExpressionList args;
        while (!PeekEq(Token::kRParen)) {
          const auto arg = ParseExpression();
          ASSERT(arg);
          args.push_back(arg);
        }
        return expr::CallProcExpr::New(expr::LiteralExpr::New(func), args);
      }
    }

    const auto cls = Class::FindClass(symbol);
    if (cls) {
      ASSERT(cls && cls->GetName()->Equals(symbol));
      ExpressionList args;
      while (!PeekEq(Token::kRParen)) {
        const auto arg = ParseExpression();
        ASSERT(arg);
        args.push_back(arg);
      }
      return expr::NewExpr::New(cls, args);
    }

    target = expr::LiteralExpr::New(symbol);
  } else {
    target = ParseExpression();
  }
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
  SetParsingArgs();
  while (!PeekEq(Token::kRBracket)) {
    const auto& next = ExpectNext(Token::kIdentifier);
    const auto idx = num_args++;
    const auto name = next.text;
    bool optional = false;
    bool vararg = false;
    switch (PeekKind()) {
      case Token::kQuestion: {
        optional = true;
        NextToken();
        break;
      }
      case Token::kDotDotDot: {
        vararg = true;
        NextToken();
        break;
      }
      case Token::kIdentifier:
      case Token::kRBracket:
        break;
      default:
        LOG(FATAL) << "invalid: " << NextToken();
    }
    args.insert(Argument(idx, name, optional, vararg));
  }
  ClearParsingArgs();
  ExpectNext(Token::kRBracket);
  return true;
}

auto Parser::ParseExpressionList(expr::ExpressionList& expressions, const bool push_scope) -> bool {
  if (push_scope)
    PushScope();
  auto peek = PeekToken();
  while (peek.kind != Token::kRParen && peek.kind != Token::kEndOfStream) {
    const auto expr = ParseExpression();
    if (expr)
      expressions.push_back(expr);
    peek = PeekToken();
  }
  if (push_scope)
    PopScope();
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

auto Parser::ParseExpression(const int depth) -> Expression* {
  {
    auto next = PeekToken();
    if (next.IsLiteral() || next.kind == Token::kIdentifier || next.kind == Token::kDispatch || next.kind == Token::kFn)
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
      case Token::kDefNamespace: {
        LOG_IF(FATAL, depth != 0) << "unexpected: " << NextToken() << ", expected: <expression>";
        const auto ns = ParseNamespace();
        ASSERT(ns);
        if (scope_)
          LOG_IF(FATAL, !scope_->Add(ns)) << "failed to add " << ns << " to scope.";
        if (script_)
          script_->Append(ns);
        if (module_)
          module_->Append(ns);
        break;
      }
      case Token::kDefMacro: {
        LocalVariable* local = nullptr;
        LOG_IF(FATAL, !ParseMacroDef(&local)) << "failed to parse macrodef.";
        ASSERT(local && local->HasValue() && local->GetValue()->IsMacro());
        if (script_)
          script_->Append(local->GetValue()->AsMacro());
        if (module_)
          module_->Append(local->GetValue()->AsMacro());
        break;
      }
      case Token::kDefNative: {
        LocalVariable* local = nullptr;
        LOG_IF(FATAL, !ParseDefNative(&local)) << "failed to parse defnative.";
        break;
      }
      case Token::kDef: {
        expr = ParseDef();
        break;
      }
      case Token::kDefn: {
        LocalVariable* local = nullptr;
        LOG_IF(FATAL, !ParseDefn(&local)) << "failed to parse local";
        break;
      }
      case Token::kNewExpr:
        expr = ParseNewExpr();
        break;
      // Expressions
      case Token::kBeginExpr:
        expr = ParseBeginExpr();
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
      case Token::kFn:
        expr = ParseLiteralLambda(next.kind);
        break;
      case Token::kLParen:
      case Token::kDispatch:
      case Token::kIdentifier: {
        expr = ParseCallExpr();
        break;
      }
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
  const auto module = Module::Find(symbol->GetFullyQualifiedName());
  LOG_IF(FATAL, !module) << "failed to find Module named `" << symbol->GetFullyQualifiedName() << "`";
  LOG_IF(FATAL, !GetScope()->Add(module->GetScope()))
      << "failed to import Module `" << symbol->GetFullyQualifiedName() << "` scope.";
  return expr::ImportExpr::New(module);
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

auto Parser::ParseIdentifier(std::string& result) -> bool {
  const auto& next = NextToken();
  if (next.kind != Token::kIdentifier) {
    result.clear();
    return Unexpected(Token::kIdentifier, next);
  }
  result = next.text;
  return true;
}

auto Parser::PeekToken() -> const Token& {
  if (!peek_.IsInvalid())
    return peek_;
  ASSERT(peek_.IsInvalid());
  return peek_ = NextToken();
}

auto Parser::IsValidIdentifierChar(const char c, const bool initial) const -> bool {
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
      return !IsParsingArgs();
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
      if (PeekChar(1) == '.' && PeekChar(2) == '.') {
        Advance(3);
        return NextToken(IsParsingArgs() ? Token::kDotDotDot : Token::kRange);
      }
      Advance(1);
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
    case ',':
      Advance();
      return NextToken(Token::kComma);
    case '{':
      Advance();
      return NextToken(Token::kLBrace);
    case '}':
      Advance();
      return NextToken(Token::kRBrace);
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
    case '?':
      Advance();
      return NextToken(Token::kQuestion);
    case '$': {
      if (IsDispatching()) {
        if (isdigit(PeekChar(1))) {
          token_len_ = 0;
          buffer_[token_len_++] = NextChar();
          while (IsValidNumberChar(PeekChar(), true)) {
            const auto next = NextChar();
            buffer_[token_len_++] = next;
          }
          const auto text = GetBufferedText();
          const auto arg_idx = static_cast<word>(atoi((const char*)&text[1]));
          ASSERT(arg_idx >= 0);
          dispatched_ = std::max(dispatched_, (arg_idx + 1));
          return NextToken(Token::kIdentifier, text);
        }
        Advance();
        const auto ident = fmt::format("${}", dispatched_++);
        return NextToken(Token::kIdentifier, ident);
      } else if (PeekChar(1) == '(') {
        Advance(2);
        return NextToken(Token::kDispatch);
      }
      break;
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
      if (PeekChar(1) == 'e' && PeekChar(2) == 'w' && PeekChar(3) == ':') {
        Advance(4);
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
    while (IsValidNumberChar(PeekChar(), whole)) {
      if (PeekChar() == '.' && !IsValidNumberChar(PeekChar(1), false))
        break;
      const auto next = NextChar();
      if (next == '.')
        whole = false;
      buffer_[token_len_++] = next;
    }
    return whole ? NextToken(Token::kLiteralLong, GetBufferedText()) : NextToken(Token::kLiteralDouble, GetBufferedText());
  } else if (IsValidIdentifierChar(next, true)) {
    token_len_ = 0;
    while (IsValidIdentifierChar(PeekChar(), token_len_ == 0)) {
      if (PeekChar() == '?') {
        if (!IsValidIdentifierChar(PeekChar(1))) {
          const auto ident = GetBufferedText();
          const auto cls = Class::FindClass(ident);
          if (!cls) {
            buffer_[token_len_++] = NextChar();
            continue;
          }
          NextChar();
          return NextToken(Token::kInstanceOfExpr, ident);
        } else if (IsParsingArgs()) {
          break;
        }
      } else if (PeekChar() == '.' && PeekChar(1) == '.') {
        break;
      }
      buffer_[token_len_++] = NextChar();
    }
    const auto ident = GetBufferedText();
    if (IsParsingArgs())
      return NextToken(Token::kIdentifier, ident);
    const auto cls = Class::FindClass(ident);
    if (cls)
      return NextToken(Token::kNewExpr, ident);
    else if (ident == "ns")
      return NextToken(Token::kDefNamespace);
    else if (ident == "def")
      return NextToken(Token::kDef);
    else if (ident == "defmacro")
      return NextToken(Token::kDefMacro);
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

auto Parser::ParseNamespace() -> Namespace* {
  ExpectNext(Token::kDefNamespace);
  const auto name = ParseSymbol();
  ASSERT(name);
  const auto scope = PushScope();
  const auto ns = Namespace::New(name, scope);
  ASSERT(ns);
  SetNamespace(ns);
  if (PeekEq(Token::kLiteralString)) {
    const auto docstring = ParseLiteralString();
    ASSERT(docstring);
    ns->SetDocs(docstring);
  }
  while (!PeekEq(Token::kRParen)) {
    ExpectNext(Token::kLParen);
    switch (PeekKind()) {
      case Token::kDefn: {
        LocalVariable* local = nullptr;
        LOG_IF(FATAL, !ParseDefn(&local)) << "failed to parse defn in " << ns;
        ASSERT(local && local->HasValue() && local->GetValue()->IsLambda());
        break;
      }
      case Token::kDefMacro: {
        LocalVariable* local = nullptr;
        LOG_IF(FATAL, !ParseMacroDef(&local)) << "failed to parse defmacro in " << ns;
        ASSERT(local && local->HasValue() && local->GetValue()->IsMacro());
        break;
      }
      case Token::kDefNative: {
        LocalVariable* local = nullptr;
        LOG_IF(FATAL, !ParseDefNative(&local)) << "failed to parse defnative in: " << ns;
        ASSERT(local && local->HasValue() && local->GetValue()->IsNativeProcedure());
        break;
      }
      default:
        Unexpected(NextToken());
        return nullptr;
    }
    ExpectNext(Token::kRParen);
  }
  ClearNamespace();
  PopScope();
  return ns;
}

auto Parser::ParseDefNative(LocalVariable** local) -> bool {
  ASSERT(local);
  ExpectNext(Token::kDefNative);
  const auto symbol = ParseSymbol();
  ASSERT(symbol);
  const auto native = NativeProcedure::FindOrCreate(symbol);
  if (!native) {
    (*local) = nullptr;
    LOG(ERROR) << "failed to find NativeProcedure w/ Symbol: " << symbol;
    return false;
  }
  // arguments
  ArgumentSet args;
  if (!ParseArguments(args)) {
    (*local) = nullptr;
    LOG(ERROR) << "failed to parse NativeProcedure arguments.";
    return false;
  }
  native->SetArgs(args);
  // docstring
  if (PeekEq(Token::kLiteralString)) {
    const auto docs = ParseLiteralString();
    ASSERT(docs);
    native->SetDocs(docs);
  }
  if (!((*local) = LocalVariable::New(GetScope(), symbol, native))) {
    (*local) = nullptr;
    LOG(ERROR) << "failed to create local for NativeProcedure: " << native;
    return false;
  }
  ASSERT((*local));
  if (!GetScope()->Add((*local))) {
    LOG(ERROR) << "failed to add local " << *(*local) << " to current scope.";
    (*local) = nullptr;
    return false;
  }
  DVLOG(1000) << "created local " << *(*local) << " for native: " << native;
  return true;
}

auto Parser::ParseMacro() -> Macro* {
  ExpectNext(Token::kDefMacro);

  const auto macro = Macro::New();
  ASSERT(macro);
  const auto scope = PushScope();
  ASSERT(scope);
  {
    // name
    Symbol* name = nullptr;
    if (PeekEq(Token::kIdentifier)) {
      name = ParseSymbol();
      ASSERT(name);
    }
    if (name)
      macro->SetSymbol(name);
    const auto local = LocalVariable::New(scope, name ? name : Symbol::New("$"), macro);
    ASSERT(local);
    LOG_IF(FATAL, !GetScope()->Add(local)) << "cannot add " << local << " to scope.";
    // arguments
    ArgumentSet args{};
    if (!ParseArguments(args))
      throw Exception("failed to parse ArgumentSet");
    macro->SetArgs(args);
    // docstring
    String* docs = nullptr;
    if (PeekEq(Token::kLiteralString)) {
      docs = ParseLiteralString();
      ASSERT(docs);
    }
    // body
    expr::ExpressionList body{};
    if (!ParseExpressionList(body, false)) {
      LOG(FATAL) << "failed to parse lambda body.";
      return nullptr;
    }
    if (docs) {
      if (body.empty()) {
        body.push_back(expr::LiteralExpr::New(docs));
      } else {
        macro->SetDocstring(docs);
      }
    }
    macro->SetBody(body);
  }
  PopScope();
  macro->SetScope(scope);
  return macro;
}

auto Parser::ParseLambda(const Token::Kind kind) -> Lambda* {
  const auto lambda = Lambda::New();
  ASSERT(lambda);
  const auto scope = PushScope();
  ASSERT(scope);
  if (kind == Token::kDispatch) {
    ExpectNext(Token::kDispatch);
    SetDispatching();

    const auto local = LocalVariable::New(scope, Symbol::New("this"), lambda);
    ASSERT(local);
    LOG_IF(FATAL, !GetScope()->Add(local)) << "cannot add " << local << " to scope.";

    expr::ExpressionList body;
    LOG_IF(FATAL, !ParseExpressionList(body)) << "failed to parse expression list.";
    lambda->SetBody(body);

    ArgumentSet args{};
    if (dispatched_ > 0) {
      for (auto idx = 0; idx < dispatched_; idx++) {
        const auto name = fmt::format("${}", idx);
        LOG_IF(FATAL, !args.insert(Argument(idx, name, false, false)).second)
            << "failed to create arg " << name << " for lambda.";
      }
    }

    lambda->SetArgs(args);
    lambda->SetScope(scope);

    PopScope();
    ClearDispatched();

    ExpectNext(Token::kRParen);
    return lambda;
  }

  ExpectNext(kind);
  lambda->SetScope(scope);
  {
    // name
    Symbol* name = nullptr;
    if (PeekEq(Token::kIdentifier)) {
      name = ParseSymbol();
      ASSERT(name);
    }
    if (name)
      lambda->SetSymbol(name);
    const auto local = LocalVariable::New(scope, name ? name : Symbol::New("$"), lambda);
    ASSERT(local);
    LOG_IF(FATAL, !GetScope()->Add(local)) << "cannot add " << local << " to scope.";
    // arguments
    ArgumentSet args{};
    if (!ParseArguments(args))
      throw Exception("failed to parse ArgumentSet");
    lambda->SetArgs(args);
    // docstring
    String* docs = nullptr;
    if (PeekEq(Token::kLiteralString)) {
      docs = ParseLiteralString();
      ASSERT(docs);
    }
    // body
    expr::ExpressionList body{};
    if (!ParseExpressionList(body, false)) {
      LOG(FATAL) << "failed to parse lambda body.";
      return nullptr;
    }
    if (docs) {
      if (body.empty()) {
        body.push_back(expr::LiteralExpr::New(docs));
      } else {
        lambda->SetDocstring(docs);
      }
    }
    lambda->SetBody(body);
  }
  PopScope();
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
  SetModule(new_module);
  expr::ExpressionList init_body{};
  while (!PeekEq(Token::kEndOfStream)) {
    const auto expr = ParseExpression();
    if (expr)
      init_body.push_back(expr);
  }

  if (!init_body.empty()) {
    const auto init = new_module->CreateInitFunc(init_body);
    ASSERT(init);
    DVLOG(1000) << "created init function for " << new_module << ": " << init;
  }

  PopScope();
  ClearModule();
  return new_module;
}

auto Parser::ParseScript() -> Script* {
  const auto scope = PushScope();
  ASSERT(scope);
  const auto script = Script::New(scope);
  ASSERT(script);
  while (!PeekEq(Token::kEndOfStream)) {
    const auto& peek = PeekToken();
    if (peek.IsLiteral() || peek.IsIdentifier() || peek.kind == Token::kFn || peek.kind == Token::kDispatch) {
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
        case Token::kDef:
          expr = ParseDef();
          break;
        case Token::kDefn: {
          LocalVariable* local = nullptr;
          LOG_IF(FATAL, !ParseDefn(&local)) << "failed to parse defn in " << script;
          ASSERT(local && local->HasValue() && local->GetValue()->IsLambda());
          script->Append(local->GetValue()->AsLambda());
          break;
        }
        case Token::kDefMacro: {
          LocalVariable* local = nullptr;
          LOG_IF(FATAL, !ParseMacroDef(&local)) << "failed to parse macrodef in " << script;
          ASSERT(local && local->HasValue() && local->GetValue()->IsMacro());
          script->Append(local->GetValue()->AsMacro());
          break;
        }
        // Expressions
        case Token::kBeginExpr:
          expr = ParseBeginExpr();
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
        case Token::kFn:
          expr = ParseLiteralLambda(next.kind);
          break;
        case Token::kLParen:
        case Token::kDispatch:
        case Token::kIdentifier: {
          expr = ParseCallExpr();
          break;
        }
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

auto Parser::ParseDefn(LocalVariable** result) -> bool {
  const auto scope = GetScope();
  const auto lambda = ParseLambda(Token::kDefn);
  ASSERT(lambda && lambda->HasSymbol());
  const auto local = LocalVariable::New(scope, lambda->GetSymbol(), lambda);
  ASSERT(local);
  LOG_IF(FATAL, !scope->Add(local)) << "failed to add " << local << " to scope.";
  (*result) = local;
  return true;
}

auto Parser::ParseMacroDef(LocalVariable** result) -> bool {
  const auto scope = GetScope();
  const auto macro = ParseMacro();
  ASSERT(macro);
  const auto local = LocalVariable::New(scope, macro->GetSymbol(), macro);
  ASSERT(local);
  LOG_IF(FATAL, !scope->Add(local)) << "failed to add " << local << " to scope.";
  (*result) = local;
  return true;
}

auto Parser::ParseDef() -> expr::Expression* {
  ExpectNext(Token::kDef);
  const auto symbol = ParseSymbol();
  ASSERT(symbol);
  const auto scope = GetScope();
  ASSERT(scope);
  const auto local = LocalVariable::New(scope, symbol);
  ASSERT(local);
  LOG_IF(FATAL, !scope->Add(local)) << "cannot add duplicate local " << (*local) << " to scope.";
  const auto value = ParseExpression();
  ASSERT(value);
  if (value->IsConstantExpr()) {
    local->SetValue(value->EvalToConstant(scope));
    return nullptr;
  }
  return expr::SetExpr::New(local, value);
}
}  // namespace gel