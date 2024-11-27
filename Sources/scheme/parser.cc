#include "scheme/parser.h"

#include <glog/logging.h>

#include "scheme/argument.h"
#include "scheme/common.h"
#include "scheme/expression.h"
#include "scheme/instruction.h"
#include "scheme/local.h"
#include "scheme/local_scope.h"
#include "scheme/object.h"
#include "scheme/token.h"

namespace scm {
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
    bindings.emplace_back(symbol, value);
    const auto local = LocalVariable::New(scope, symbol);
    ASSERT(local);
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
  uint64_t num_args = 0;
  while (PeekEq(Token::kIdentifier)) {
    const auto& next = NextToken();
    ASSERT(next.kind == Token::kIdentifier);
    args.insert(Argument(num_args++, next.text));
  }
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

auto Parser::ParseLambdaExpr() -> LambdaExpr* {
  ExpectNext(Token::kLambdaExpr);
  ExpectNext(Token::kLParen);
  ArgumentSet args;
  if (!ParseArguments(args)) {
    LOG(FATAL) << "failed to parse lambda arguments.";
    return nullptr;
  }
  ExpectNext(Token::kRParen);
  PushScope();
  ExpressionList body;
  if (!ParseExpressionList(body)) {
    LOG(FATAL) << "failed to parse lambda body.";
    return nullptr;
  }
  PopScope();
  return LambdaExpr::New(args, body);
}

auto Parser::ParseSetExpr() -> SetExpr* {
  ExpectNext(Token::kSetExpr);
  const auto symbol = ParseSymbol();
  ASSERT(symbol);
  const auto value = ParseExpression();
  ASSERT(value);
  return SetExpr::New(symbol, value);
}

auto Parser::ParseEvalExpr() -> expr::EvalExpr* {
  ExpectNext(Token::kEvalExpr);
  const auto value = ParseExpression();
  ASSERT(value);
  return EvalExpr::New(value);
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
    expr = ParseBinaryExpr();
  } else if (next.IsLiteral()) {
    expr = ParseListExpr();
  } else {
    switch (next.kind) {
      case Token::kDefine: {
        expr = ParseLocalDef();
        break;
      }
      // Definitions
      case Token::kMacroDef:
        expr = ParseMacroDef();
        break;
      case Token::kDefun:
        expr = ParseDefunExpr();
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
      case Token::kEvalExpr:
        expr = ParseEvalExpr();
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

  ExpectNext(Token::kLParen);
  ArgumentSet args;
  if (!ParseArguments(args)) {
    LOG(ERROR) << "failed to parse macro args.";
    return nullptr;
  }
  ExpectNext(Token::kRParen);

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

auto Parser::ParseDefunExpr() -> LocalDef* {
  ExpectNext(Token::kDefun);
  const auto symbol = ParseSymbol();
  ASSERT(symbol);
  ExpectNext(Token::kLParen);
  ArgumentSet args;
  if (!ParseArguments(args)) {
    LOG(FATAL) << "failed to parse lambda arguments.";
    return nullptr;
  }
  ExpectNext(Token::kRParen);

  PushScope();
  ExpressionList body;
  if (!ParseExpressionList(body)) {
    LOG(FATAL) << "failed to parse lambda body.";
    return nullptr;
  }
  PopScope();
  return LocalDef::New(symbol, LambdaExpr::New(args, body));
}

auto Parser::ParseLocalDef() -> LocalDef* {
  ExpectNext(Token::kDefine);
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

auto Parser::ParseDefinition() -> expr::Definition* {
  ExpectNext(Token::kLParen);
  expr::Definition* defn = nullptr;
  const auto& next = PeekToken();
  switch (next.kind) {
    case Token::kDefun:
      defn = ParseDefunExpr();
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
      return NextToken(Token::kDefine);
    else if (ident == "defmacro")
      return NextToken(Token::kMacroDef);
    else if (ident == "import")
      return NextToken(Token::kImportDef);
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
    else if (ident == "lambda")
      return NextToken(Token::kLambdaExpr);
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
    else if (ident == "set!")
      return NextToken(Token::kSetExpr);
    else if (ident == "cond")
      return NextToken(Token::kCond);
    else if (ident == "eval")
      return NextToken(Token::kEvalExpr);
    else if (ident == "when")
      return NextToken(Token::kWhenExpr);
    else if (ident == "case")
      return NextToken(Token::kCaseExpr);
    else if (ident == "while")
      return NextToken(Token::kWhileExpr);
    else if (ident == "defun")
      return NextToken(Token::kDefun);
    else if (ident == "let")
      return NextToken(Token::kLetExpr);
    return NextToken(Token::kIdentifier, ident);
  }

  return NextToken(Token::kInvalid, GetRemaining());
}

auto Parser::ParseLocalVariable() -> LocalVariable* {
  ExpectNext(Token::kDefine);
  const auto symbol = ParseSymbol();
  ASSERT(symbol);
  const auto value = ParseExpression();
  ASSERT(value);
  const auto local = LocalVariable::New(GetScope(), symbol, value->IsConstantExpr() ? value->EvalToConstant() : nullptr);
  ASSERT(local);
  // TODO: store value if not constant
  if (!GetScope()->Add(local))
    throw Exception(fmt::format("failed to add LocalVariable: {}", local->GetName()));
  return local;
}

auto Parser::ParseNamedLambda() -> Lambda* {
  ExpectNext(Token::kDefun);
  // name
  const auto name = ParseSymbol();
  ASSERT(name);
  LOG_IF(WARNING, GetScope()->Has(name)) << "redefining: " << name;
  // arguments
  ExpectNext(Token::kLParen);
  ArgumentSet args;
  if (!ParseArguments(args))
    throw Exception("failed to parse ArgumentSet");
  ExpectNext(Token::kRParen);
  // body TODO: allow (<expression list>)
  PushScope();
  ExpressionList body;
  if (!ParseExpressionList(body)) {
    LOG(FATAL) << "failed to parse lambda body.";
    return nullptr;
  }
  PopScope();

  const auto lambda = Lambda::New(args, body);
  ASSERT(lambda);
  lambda->SetName(name);
  return lambda;
}

auto Parser::ParseListExpr() -> expr::ListExpr* {
  const auto list = expr::ListExpr::New();
  while (!PeekEq(Token::kRParen)) {
    list->Append(ParseExpression());
  }
  return list;
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
        // Definitions
        case Token::kDefine: {
          const auto local = ParseLocalVariable();
          ASSERT(local);
          break;
        }
        case Token::kDefun: {
          const auto lambda = ParseNamedLambda();
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
        case Token::kLambdaExpr:
          expr = ParseLambdaExpr();
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
        case Token::kEvalExpr:
          expr = ParseEvalExpr();
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
}  // namespace scm