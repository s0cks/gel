#include "gel/parser.h"

#include <glog/logging.h>

#include <unordered_map>
#include <utility>

#include "gel/argument.h"
#include "gel/common.h"
#include "gel/expr/expression.h"
#include "gel/instruction.h"
#include "gel/lambda.h"
#include "gel/local.h"
#include "gel/local_scope.h"
#include "gel/macro.h"
#include "gel/module.h"
#include "gel/native_procedure.h"
#include "gel/object.h"
#include "gel/token.h"
#include "gel/tracing.h"
#include "gel/type_traits.h"

namespace gel {
static KeywordTrie::Node* keywords_ = new KeywordTrie::Node();

static inline void RegisterKeyword(const std::string& keyword, const Token::Kind kind) {
  LOG_IF(FATAL, !KeywordTrie::Insert(keywords_, keyword, kind))
      << "failed to define keyword: " << keyword << " (" << kind << ").";
}

#define CHECK_RESULT(Expr)      \
  ({                            \
    const auto status = (Expr); \
    if (!status) {              \
      DLOG(ERROR) << status;    \
      return status;            \
    }                           \
  })

#define EXPECT(Token, Kind)   \
  if ((Token).kind != (Kind)) \
    return UnexpectedError((Token), (Kind));

#define EXPECT_NEXT(Kind)           \
  ({                                \
    const auto& next = NextToken(); \
    EXPECT(next, Kind);             \
  })

class ParseScope {
  DEFINE_NON_COPYABLE_TYPE(ParseScope);

 private:
  Parser* parser_;
  LocalScope* scope_{};

 public:
  ParseScope(Parser* parser) :
    parser_(parser) {
    ASSERT(parser_);
    scope_ = parser_->PushScope();
  }
  ~ParseScope() {
    ASSERT(parser_);
    parser_->PopScope();
  }

  auto operator->() const -> LocalScope* {
    return scope_;
  }

  operator LocalScope*() const {
    return scope_;
  }
};

template <class T>
auto Parser::TryParseDocstring(T* owner, std::enable_if_t<gel::has_docs<T>::value>*) -> ParseResult {
  ASSERT(owner);
  if (PeekEq(Token::kLiteralString)) {
    String* docstring = nullptr;
    CHECK_RESULT(ParseLiteralString(&docstring));
    ASSERT(docstring);
    owner->SetDocs(docstring);
  }
  return true;
}

template auto Parser::TryParseDocstring(Lambda*, void*) -> ParseResult;
template auto Parser::TryParseDocstring(Namespace*, void*) -> ParseResult;

template <class T>
auto Parser::TryParseSymbol(T* owner, std::enable_if_t<gel::has_symbol<T>::value>*) -> ParseResult {
  ASSERT(owner);
  Symbol* symbol = nullptr;
  if (PeekEq(Token::kIdentifier)) {
    CHECK_RESULT(ParseLiteralSymbol(&symbol));
    ASSERT(symbol);
    owner->SetSymbol(symbol);
  }
  return true;
}

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

auto Parser::ParseLiteralString(String** result) -> ParseResult {
  const auto& next = NextToken();
  EXPECT(next, Token::kLiteralString);
  (*result) = next.IsEmpty() ? String::Empty() : String::New(next.text);
  return true;
}

auto Parser::ParseLiteralSymbol(Symbol** result) -> ParseResult {
  const auto& next = NextToken();  // TODO: fix the weird logic where kNewExpr check is needed
  if (next.kind != Token::kIdentifier && next.kind != Token::kNewExpr) {
    std::stringstream ss;
    ss << "unexpected " << next << ", expected a Symbol or new-expr.";
    return ReturnError(ss, next.pos);
  }
  ASSERT(next.kind == Token::kIdentifier || next.kind == Token::kNewExpr);
  ASSERT(!next.text.empty());
  if (HasOwner() && GetOwner()->IsNamespace()) {
    (*result) = GetOwner()->AsNamespace()->CreateSymbol(next.text);
    return true;
  }
  (*result) = Symbol::New(next.text);
  return true;
}

auto Parser::ParseLiteralLambda(const Token::Kind kind, expr::LiteralExpr** result) -> ParseResult {
  Lambda* lambda = nullptr;
  CHECK_RESULT(ParseLambda(kind, &lambda));
  ASSERT(lambda);
  const auto literal = expr::LiteralExpr::New(lambda);
  ASSERT(literal);
  (*result) = literal;
  return true;
}

auto Parser::ParseMap(expr::Expression** result) -> ParseResult {
  EXPECT_NEXT(Token::kLBrace);

  Symbol* key = nullptr;
  expr::Expression* value = nullptr;

  expr::NewMapExpr::EntryList data{};
  while (!PeekEq(Token::kRBrace)) {
    CHECK_RESULT(ParseLiteralSymbol(&key));
    ASSERT(key);
    CHECK_RESULT(ParseExpression(&value));
    ASSERT(value);
    if (PeekEq(Token::kComma))
      NextToken();
    data.emplace_back(key, value);
  }
  EXPECT_NEXT(Token::kRBrace);
  (*result) = expr::NewMapExpr::New(data);
  return true;
}

auto Parser::ParseLiteralBool(Bool** result) -> ParseResult {
  const auto& next = NextToken();
  switch (next.kind) {
    case Token::kLiteralTrue: {
      (*result) = Bool::True();
      break;
    }
    case Token::kLiteralFalse:
      (*result) = Bool::False();
      break;
    default:
      return UnexpectedError(next);
  }
  return true;
}

auto Parser::ParseLiteralNumber(Number** result) -> ParseResult {
  const auto& next = NextToken();
  switch (next.kind) {
    case Token::kLiteralLong: {
      (*result) = Long::New(next.AsLong());
      return true;
    }
    case Token::kLiteralDouble: {
      (*result) = Double::New(next.AsDouble());
      return true;
    }
    default:
      return UnexpectedError(next);
  }
}

auto Parser::ParseLiteralValue(Object** result) -> ParseResult {
  switch (PeekKind()) {
    case Token::kLiteralFalse:
    case Token::kLiteralTrue:
      return ParseLiteralBool((Bool**)result);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
    case Token::kLiteralLong:
    case Token::kLiteralDouble:
      return ParseLiteralNumber((Number**)result);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
    case Token::kLiteralString:
      return ParseLiteralString((String**)result);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
    case Token::kIdentifier:
      return ParseLiteralSymbol((Symbol**)result);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
    default:
      return UnexpectedError(NextToken());
  }
}

auto Parser::ParseLiteralExpr(expr::Expression** result) -> ParseResult {
  if (PeekEq(Token::kFn) || PeekEq(Token::kDispatch)) {
    return ParseLiteralLambda(PeekKind(), (expr::LiteralExpr**)result);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  } else if (PeekEq(Token::kLBrace)) {
    return ParseMap(result);
  }

  Object* literal = nullptr;
  CHECK_RESULT(ParseLiteralValue(&literal));
  ASSERT(literal);
  (*result) = expr::LiteralExpr::New(literal);
  return true;
}

auto Parser::ParseBeginExpr(expr::Expression** result) -> ParseResult {
  ParseScope scope(this);
  EXPECT_NEXT(Token::kBeginExpr);
  expr::ExpressionList body{};
  expr::Expression* expr = nullptr;
  while (!PeekEq(Token::kRParen)) {
    CHECK_RESULT(ParseExpression(&expr));
    ASSERT(expr);
    body.push_back(expr);
  }
  ASSERT(PeekEq(Token::kRParen));
  (*result) = expr::BeginExpr::New(body);
  return true;
}

static inline auto IsClassReference(expr::Expression* expr) -> bool {
  if (IsLiteralSymbol(expr)) {
    const auto symbol = expr->AsLiteralExpr()->GetValue()->AsSymbol();
    ASSERT(symbol);
    return Class::FindClass(symbol) != nullptr;
  }
  return false;
}

static inline auto IsNativeCall(LocalScope* scope, expr::Expression* expr, NativeProcedure** target) -> bool {
  ASSERT(scope);
  ASSERT(expr);
  if (!expr->IsLiteralExpr()) {
    (*target) = nullptr;
    return false;
  }

  const auto literal = expr->AsLiteralExpr();
  if (literal->GetValue()->IsNativeProcedure()) {
    (*target) = literal->GetValue()->AsNativeProcedure();
    return true;
  } else if (literal->GetValue()->IsSymbol()) {
    const auto symbol = literal->GetValue()->AsSymbol();
    ASSERT(symbol);
    LocalVariable* local = nullptr;
    if (!scope->Lookup(symbol, &local)) {
      (*target) = nullptr;
      return false;
    }
    if (!local->HasValue() || !local->GetValue()->IsNativeProcedure()) {
      (*target) = nullptr;
      return false;
    }
    (*target) = local->GetValue()->AsNativeProcedure();
    return true;
  }
  (*target) = nullptr;
  return false;
}

static inline auto IsMacroCall(LocalScope* scope, expr::Expression* expr, Macro** target) -> bool {
  ASSERT(scope);
  ASSERT(expr);
  if (!expr->IsLiteralExpr()) {
    (*target) = nullptr;
    return false;
  }

  const auto literal = expr->AsLiteralExpr();
  if (literal->GetValue()->IsMacro()) {
    (*target) = literal->GetValue()->AsMacro();
    return true;
  } else if (literal->GetValue()->IsSymbol()) {
    const auto symbol = literal->GetValue()->AsSymbol();
    ASSERT(symbol);
    LocalVariable* local = nullptr;
    if (!scope->Lookup(symbol, &local)) {
      (*target) = nullptr;
      return false;
    }
    if (!local->HasValue() || !local->GetValue()->IsMacro()) {
      (*target) = nullptr;
      return false;
    }
    (*target) = local->GetValue()->AsMacro();
    return true;
  }
  (*target) = nullptr;
  return false;
}

static inline auto IsCallable(LocalVariable* local) -> bool {
  return local && local->HasValue() && (local->GetValue()->IsProcedure() || local->GetValue()->IsMacro());
}

auto Parser::ParseCallExpr(expr::Expression** result) -> ParseResult {
  Expression* target = nullptr;
  if (PeekEq(Token::kIdentifier)) {
    Symbol* symbol = nullptr;
    CHECK_RESULT(ParseLiteralSymbol(&symbol));
    ASSERT(symbol);
    if (symbol->HasSymbolType()) {
      const auto cls = Class::FindClass(symbol->GetSymbolType());
      if (cls) {
        ASSERT(cls);
        const auto func = cls->FindFunction(symbol);
        if (func) {
          expr::ExpressionList args{};
          expr::Expression* arg = nullptr;
          while (!PeekEq(Token::kRParen)) {
            CHECK_RESULT(ParseExpression(&arg));
            ASSERT(arg);
            args.push_back(arg);
          }
          (*result) = expr::InvokeExpr::New(expr::LiteralExpr::New(func), args);
          return true;
        }
        const auto field = cls->FindField(symbol);
        if (field) {
          expr::Expression* instance = nullptr;
          CHECK_RESULT(ParseExpression(&instance));
          ASSERT(instance);
          (*result) = expr::LoadFieldExpr::New(instance, field);
          return true;
        }
      }

      LocalVariable* local = nullptr;
      if (GetScope()->Lookup(symbol->GetSymbolType(), &local) && local && local->HasValue()) {
        const auto type = local->GetValue()->GetType();
        const auto func = type->FindFunction(symbol->GetSymbolName());
        if (func) {
          expr::ExpressionList args{};
          expr::Expression* arg = nullptr;
          while (!PeekEq(Token::kRParen)) {
            CHECK_RESULT(ParseExpression(&arg));
            ASSERT(arg);
            args.push_back(arg);
          }
          (*result) = expr::InvokeInstanceExpr::New(func, expr::LiteralExpr::New(local->GetSymbol()), args);
          return true;
        }
      }
    }

    const auto cls = Class::FindClass(symbol);
    if (cls) {
      ASSERT(cls && cls->GetName()->Equals(symbol));
      expr::ExpressionList args{};
      expr::Expression* arg = nullptr;
      while (!PeekEq(Token::kRParen)) {
        CHECK_RESULT(ParseExpression(&arg));
        ASSERT(arg);
        args.push_back(arg);
      }
      (*result) = expr::NewExpr::New(cls, args);
      return true;
    }

    LocalVariable* local = nullptr;
    if (GetScope()->Lookup(symbol, &local) && IsCallable(local)) {
      target = expr::LiteralExpr::New(local->GetValue());
    } else {
      target = expr::LiteralExpr::New(symbol);
    }
  } else {
    CHECK_RESULT(ParseExpression(&target));
  }
  ASSERT(target);

  expr::ExpressionList args{};
  expr::Expression* arg = nullptr;
  while (!PeekEq(Token::kRParen)) {
    CHECK_RESULT(ParseExpression(&arg));
    ASSERT(arg);
    args.push_back(arg);
  }

  const auto scope = GetScope();
  if (target->IsLiteralExpr()) {
    const auto literal = target->AsLiteralExpr();
    ASSERT(literal);
    if (literal->GetValue()->IsMacro()) {
      (*result) = expr::InvokeMacroExpr::New(literal->GetValue()->AsMacro(), args);
      return true;
    } else if (literal->GetValue()->IsNativeProcedure()) {
      (*result) = expr::InvokeNativeExpr::New(literal->GetValue()->AsNativeProcedure(), args);
      return true;
    } else if (literal->GetValue()->IsSymbol()) {
      const auto symbol = literal->GetValue()->AsSymbol();
      ASSERT(symbol);
      LocalVariable* local = nullptr;
      if (scope->Lookup(symbol, &local) && local && local->HasValue()) {
        if (local->GetValue()->IsMacro()) {
          (*result) = expr::InvokeMacroExpr::New(literal->GetValue()->AsMacro(), args);
          return true;
        } else if (local->GetValue()->IsNativeProcedure()) {
          (*result) = expr::InvokeNativeExpr::New(literal->GetValue()->AsNativeProcedure(), args);
          return true;
        }
      }
    }
  }
  (*result) = expr::InvokeExpr::New(target, args);
  return true;
}

auto Parser::ParseUnaryOpExpr(expr::Expression** result) -> ParseResult {
  const auto& next = NextToken();
  const auto op = next.ToUnaryOp();
  if (!op)
    return Unexpected(next);
  expr::Expression* value = nullptr;
  CHECK_RESULT(ParseExpression(&value));
  ASSERT(value);
  (*result) = expr::UnaryOpExpr::New((*op), value);
  return true;
}

auto Parser::ParseBinaryExpr(expr::Expression** result) -> ParseResult {
  const auto op = NextToken().ToBinaryOp();
  ASSERT(op);
  expr::Expression* left_expr = nullptr;
  CHECK_RESULT(ParseExpression(&left_expr));
  ASSERT(left_expr);
  expr::Expression* right_expr = nullptr;
  CHECK_RESULT(ParseExpression(&right_expr));
  ASSERT(right_expr);
  do {
    left_expr = BinaryOpExpr::New((*op), left_expr, right_expr);
    ASSERT(left_expr);
    if (PeekEq(Token::kRParen))
      break;
    CHECK_RESULT(ParseExpression(&right_expr));
    ASSERT(right_expr);
  } while (true);
  ASSERT(left_expr && left_expr->IsBinaryOpExpr());
  (*result) = left_expr->AsBinaryOpExpr();
  return true;
}

auto Parser::ParseCondExpr(expr::Expression** result) -> ParseResult {
  const auto start_pos = GetPos();
  EXPECT_NEXT(Token::kCond);
  expr::ClauseList clauses;
  expr::Expression* a = nullptr;
  expr::Expression* b = nullptr;
  expr::Expression* alt = nullptr;
  do {
    CHECK_RESULT(ParseExpression(&a));
    ASSERT(a);
    if (PeekEq(Token::kRParen)) {
      alt = a;
      break;
    }
    CHECK_RESULT(ParseExpression(&b));
    ASSERT(b);
    clauses.push_back(expr::ClauseExpr::New(a, b));
  } while (!PeekEq(Token::kRParen));
  (*result) = CondExpr::New(clauses, alt);
  return true;
}

auto Parser::ParseRxOpExpr(expr::Expression** result) -> ParseResult {
  EXPECT_NEXT(Token::kLParen);
  Symbol* symbol = nullptr;
  CHECK_RESULT(ParseLiteralSymbol(&symbol));
  ASSERT(symbol);
  expr::ExpressionList args{};
  CHECK_RESULT(ParseExpressionList(args));
  EXPECT_NEXT(Token::kRParen);
  (*result) = expr::RxOpExpr::New(symbol, args);
  return true;
}

auto Parser::ParseRxOpList(expr::RxOpList& operators) -> ParseResult {  // TODO: refactor & better error handling
  expr::Expression* expr = nullptr;
  auto peek = PeekToken();
  while (peek.kind != Token::kRParen && peek.kind != Token::kEndOfStream) {
    CHECK_RESULT(ParseRxOpExpr(&expr));
    ASSERT(expr);
    operators.push_back(expr->AsRxOpExpr());  // TODO: this is an unchecked cast
    peek = PeekToken();
    expr = nullptr;
  }
  return true;
}

auto Parser::ParseLetRxExpr(expr::Expression** result) -> ParseResult {
  EXPECT_NEXT(Token::kLetRxExpr);
  ParseScope scope(this);
  expr::Expression* observable = nullptr;
  CHECK_RESULT(ParseExpression(&observable));
  ASSERT(observable);
  expr::RxOpList operators{};
  if (PeekEq(Token::kRParen)) {
    (*result) = LetRxExpr::New(scope, observable, operators);
    return true;
  }
  CHECK_RESULT(ParseRxOpList(operators));
  (*result) = LetRxExpr::New(scope, observable, operators);
  return true;
}

auto Parser::ParseLetExpr(expr::Expression** result) -> ParseResult {
  EXPECT_NEXT(Token::kLetExpr);
  const auto scope = PushScope();
  // parse bindings
  expr::Expression* value = nullptr;
  expr::BindingList bindings;
  EXPECT_NEXT(Token::kLParen);
  while (!PeekEq(Token::kRParen)) {
    EXPECT_NEXT(Token::kLParen);
    Symbol* symbol = nullptr;
    CHECK_RESULT(ParseLiteralSymbol(&symbol));
    ASSERT(symbol);
    if (scope->Has(symbol)) {
      std::stringstream ss;
      ss << "cannot redefine binding for: " << symbol;
      return ReturnError(ss, pos_);
    }
    CHECK_RESULT(ParseExpression(&value));
    ASSERT(value);
    const auto local = LocalVariable::New(scope, symbol);
    ASSERT(local);
    if (!scope->Add(local)) {
      std::stringstream ss;
      ss << "failed to add " << (*local) << " to current scope.";
      return ReturnError(ss, pos_);
    }
    bindings.emplace_back(Binding::New(local, value));
    EXPECT_NEXT(Token::kRParen);
  }
  EXPECT_NEXT(Token::kRParen);
  // parse body
  ExpressionList body{};
  CHECK_RESULT(ParseExpressionList(body));
  PopScope();
  (*result) = LetExpr::New(scope, bindings, body);
  return true;
}

auto Parser::ParseArguments(Array<Argument*>** args, const bool bind) -> ParseResult {  // TODO: remove bind?
  ExpectNext(Token::kLBracket);
  const auto scope = GetScope();
  ASSERT(scope);
  std::vector<Argument*> parsed_args{};
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
    parsed_args.push_back(Argument::New(idx, String::New(name), optional, vararg));
    if (bind) {
      const auto local = LocalVariable::New(scope, Symbol::New(name));
      ASSERT(local);
      if (!scope->Add(local)) {
        std::stringstream ss;
        ss << "failed to add " << (*local) << " to current scope.";
        return ReturnError(ss, pos_);
      }
    }
  }
  if (num_args > 0) {
    ASSERT(!parsed_args.empty());
    const auto array = Array<Argument*>::New(static_cast<word>(parsed_args.size()));
    ASSERT(array);
    for (const auto& arg : parsed_args) {
      array->Push(arg);
    }
    (*args) = array;
  }
  ClearParsingArgs();
  ExpectNext(Token::kRBracket);
  return true;
}

auto Parser::ParseExpressionList(expr::ExpressionList& expressions, const bool push_scope) -> ParseResult {
  if (push_scope)
    PushScope();
  auto peek = PeekToken();
  expr::Expression* value = nullptr;
  while (peek.kind != Token::kRParen && peek.kind != Token::kEndOfStream) {
    CHECK_RESULT(ParseExpression(&value));
    if (value)
      expressions.push_back(value);
    peek = PeekToken();
  }
  if (push_scope)
    PopScope();
  return true;
}

auto Parser::ParseThrowExpr(expr::Expression** result) -> ParseResult {
  EXPECT_NEXT(Token::kThrowExpr);
  expr::Expression* value = nullptr;
  CHECK_RESULT(ParseExpression(&value));
  ASSERT(value);
  (*result) = ThrowExpr::New(value);
  return true;
}

auto Parser::ParseSetExpr(expr::Expression** result) -> ParseResult {
  const auto start_pos = GetPos();
  EXPECT_NEXT(Token::kSet);
  Symbol* symbol = nullptr;
  CHECK_RESULT(ParseLiteralSymbol(&symbol));
  ASSERT(symbol);
  if (symbol->HasSymbolType()) {
    const auto cls = Class::FindClass(symbol->GetSymbolType());
    if (cls) {
      ASSERT(cls);
      const auto field = cls->FindField(symbol);
      if (field) {
        ASSERT(field);
        expr::Expression* instance = nullptr;
        CHECK_RESULT(ParseExpression(&instance));
        ASSERT(instance);
        expr::Expression* value = nullptr;
        CHECK_RESULT(ParseExpression(&value));
        ASSERT(value);
        (*result) = expr::StoreFieldExpr::New(field, instance, value);
        return true;
      }
    }
  }

  const auto scope = GetScope();
  ASSERT(scope);
  LocalVariable* local = nullptr;
  if (!scope->Lookup(symbol, &local)) {
    DLOG(WARNING) << "failed to find local named `" << symbol << "`";
    local = LocalVariable::New(scope, symbol, nullptr);
    ASSERT(local);
    if (!scope->Add(local)) {
      std::stringstream ss;
      ss << "failed to add " << (*local) << " to current scope.";
      (*result) = nullptr;
      return ReturnError(ss, start_pos);
    }
  }
  expr::Expression* value = nullptr;
  CHECK_RESULT(ParseExpression(&value));
  ASSERT(value);
  (*result) = expr::StoreLocalExpr::New(local, value);
  return true;
}

auto Parser::ParseExpression(expr::Expression** result, const int depth) -> ParseResult {
  TRACE_ZONE_NAMED("Parser::ParseExpression");
  {
    auto next = PeekToken();
    if (next.IsLiteral()) {
      CHECK_RESULT(ParseLiteralExpr(result));
      return true;
    } else if (next.kind == Token::kQuote) {
      CHECK_RESULT(ParseQuotedExpr(result));
      return true;
    }
  }

  EXPECT_NEXT(Token::kLParen);
  const auto next = PeekToken();
  if (next.IsUnaryOp()) {
    CHECK_RESULT(ParseUnaryOpExpr(result));
  } else if (next.IsBinaryOp()) {
    CHECK_RESULT(ParseBinaryExpr(result));
  } else if (next.IsLiteral() && !(next.IsSymbol() || next.IsFunctionLiteral())) {
    CHECK_RESULT(ParseListExpr(result));
  } else {
    switch (next.kind) {
      case Token::kDefNamespace: {
        LocalVariable* local = nullptr;
        CHECK_RESULT(ParseDefNamespace(&local));
        ASSERT(local && local->GetValue()->IsNamespace());
        break;
      }
      case Token::kDefMacro: {
        LocalVariable* local = nullptr;
        CHECK_RESULT(ParseDefMacro(&local));
        ASSERT(local && local->HasValue() && local->GetValue()->IsMacro());
        break;
      }
      case Token::kDefNative: {
        LocalVariable* local = nullptr;
        CHECK_RESULT(ParseDefNative(&local));
        ASSERT(local && local->HasValue() && local->GetValue()->IsNativeProcedure());
        break;
      }
      case Token::kDef: {
        CHECK_RESULT(ParseDef(result));
        break;
      }
      case Token::kDefn: {
        LocalVariable* local = nullptr;
        CHECK_RESULT(ParseDefn(&local));
        break;
      }
      case Token::kNewExpr: {
        CHECK_RESULT(ParseNewExpr(result));
        break;
      }
      case Token::kBeginExpr: {
        CHECK_RESULT(ParseBeginExpr(result));
        break;
      }
      case Token::kSet: {
        CHECK_RESULT(ParseSetExpr(result));
        break;
      }
      case Token::kCond: {
        CHECK_RESULT(ParseCondExpr(result));
        break;
      }
      case Token::kThrowExpr: {
        CHECK_RESULT(ParseThrowExpr(result));
        break;
      }
      case Token::kFn: {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
        CHECK_RESULT(ParseLiteralLambda(next.kind, (expr::LiteralExpr**)result));
        break;
      }
      case Token::kLParen:
      case Token::kDispatch:
      case Token::kIdentifier: {
        CHECK_RESULT(ParseCallExpr(result));
        break;
      }
      case Token::kQuote: {
        CHECK_RESULT(ParseQuotedExpr(result));
        break;
      }
      case Token::kWhenExpr: {
        CHECK_RESULT(ParseWhenExpr(result));
        break;
      }
      case Token::kCaseExpr: {
        CHECK_RESULT(ParseCaseExpr(result));
        break;
      }
      case Token::kWhileExpr: {
        CHECK_RESULT(ParseWhileExpr(result));
        break;
      }
      case Token::kLetRxExpr: {
        CHECK_RESULT(ParseLetRxExpr(result));
        break;
      }
      case Token::kCastExpr: {
        CHECK_RESULT(ParseCastExpr(result));
        break;
      }
      case Token::kInstanceOfExpr: {
        CHECK_RESULT(ParseInstanceOfExpr(result));
        break;
      }
      case Token::kLetExpr: {
        CHECK_RESULT(ParseLetExpr(result));
        break;
      }
      case Token::kRParen: {
        NextToken();
        return expr::ListExpr::New();
      }
      default:
        return UnexpectedError(next);
    }
  }
  EXPECT_NEXT(Token::kRParen);
  return true;
}

auto Parser::ParseQuotedExpr(expr::Expression** result) -> ParseResult {
  const auto depth = GetDepth();
  EXPECT_NEXT(Token::kQuote);
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
  if (text == "()") {
    (*result) = expr::LiteralExpr::New(Pair::Empty());
    return true;
  }
  (*result) = expr::QuotedExpr::New(GetBufferedText());
  return true;
}

auto Parser::ParseImportExpr(expr::Expression** result) -> ParseResult {
  const auto start_pos = GetPos();
  EXPECT_NEXT(Token::kImportExpr);
  const auto& next = NextToken();
  EXPECT(next, Token::kLiteralString);
  const auto target_module = Module::FindOrLoad(next.text);
  if (!target_module) {
    const auto& ident = next.text;
    std::stringstream ss;
    ss << "failed to import Module from `" << ident << "`";
    (*result) = nullptr;
    return ReturnError(ss, start_pos);
  }
  GetScope()->AddAll(target_module->GetScope());
  (*result) = expr::ImportExpr::New(target_module);
  return true;
}

auto Parser::ParseWhenExpr(expr::Expression** result) -> ParseResult {
  EXPECT_NEXT(Token::kWhenExpr);
  expr::Expression* test = nullptr;
  CHECK_RESULT(ParseExpression(&test));
  ASSERT(test);
  ExpressionList actions{};
  CHECK_RESULT(ParseExpressionList(actions));
  (*result) = expr::WhenExpr::New(test, actions);
  return true;
}

auto Parser::ParseClauseList(expr::ClauseList& clauses) -> ParseResult {
  auto peek = PeekToken();
  while (peek.kind != Token::kRParen && peek.kind != Token::kEndOfStream) {
    EXPECT_NEXT(Token::kLParen);
    expr::Expression* key = nullptr;
    CHECK_RESULT(ParseLiteralExpr(&key));
    ASSERT(key);
    expr::ExpressionList actions{};
    CHECK_RESULT(ParseExpressionList(actions));
    clauses.push_back(ClauseExpr::New(key, actions));
    EXPECT_NEXT(Token::kRParen);
    peek = PeekToken();
  }
  return true;
}

auto Parser::ParseCaseExpr(expr::Expression** result) -> ParseResult {
  EXPECT_NEXT(Token::kCaseExpr);
  expr::Expression* key = nullptr;
  CHECK_RESULT(ParseExpression(&key));
  ASSERT(key);
  expr::ClauseList clauses{};
  CHECK_RESULT(ParseClauseList(clauses));
  (*result) = expr::CaseExpr::New(key, clauses);
  return true;
}

auto Parser::ParseWhileExpr(expr::Expression** result) -> ParseResult {
  EXPECT_NEXT(Token::kWhileExpr);
  expr::Expression* test = nullptr;
  CHECK_RESULT(ParseExpression(&test));
  ASSERT(test);
  ExpressionList body{};
  CHECK_RESULT(ParseExpressionList(body));
  (*result) = expr::WhileExpr::New(test, body);
  return true;
}

auto Parser::ParseNewExpr(expr::Expression** result) -> ParseResult {
  const auto new_expr_token = NextToken();
  EXPECT(new_expr_token, Token::kNewExpr);
  const auto symbol = Symbol::New(new_expr_token.text);
  ASSERT(symbol);
  const auto cls = Class::FindClass(symbol);
  if (!cls) {
    std::stringstream ss;
    ss << "failed to find Class w/ symbol: " << symbol;
    return ReturnError(ss, new_expr_token.pos);
  }
  expr::ExpressionList args{};
  CHECK_RESULT(ParseExpressionList(args));
  (*result) = expr::NewExpr::New(cls, args);
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
    if (!IsDoubleQuote(PeekChar()))
      return NextToken(Token::kInvalid, GetBufferedText());
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
    auto ckw = keywords_;
    while (IsValidIdentifierChar(PeekChar(), token_len_ == 0)) {
      if (PeekChar() == '?') {
        if (!IsValidIdentifierChar(PeekChar(1))) {
          const auto ident = GetBufferedText();
          const auto cls = Class::FindClass(ident);
          if (cls) {
            NextChar();
            return NextToken(Token::kInstanceOfExpr, ident);
          }
        } else if (IsParsingArgs()) {
          break;
        }
      } else if (PeekChar() == '.' && PeekChar(1) == '.') {
        break;
      }
      const auto c = NextChar();
      buffer_[token_len_++] = c;
      if (!ckw || ckw->children.at(c) == nullptr) {
        ckw = nullptr;
        continue;
      }
      ckw = ckw->children.at(c);
    }
    if (ckw && !ckw->epsilon)
      ckw = nullptr;
    const auto ident = GetBufferedText();
    LOG_IF(FATAL, ident == "eq?" && ckw == nullptr) << "checking eq";
    if (IsParsingArgs())
      return NextToken(Token::kIdentifier, ident);
    if (ckw) {
      ASSERT(ckw->epsilon);
      return NextToken(ckw->kind, ident);
    }
    return NextToken(Token::kIdentifier, ident);
  }

  return NextToken(Token::kInvalid, GetRemaining());
}

auto Parser::ParseCastExpr(expr::Expression** result) -> ParseResult {
  const auto start_pos = GetPos();
  const auto token = NextToken();
  EXPECT(token, Token::kCastExpr);
  ASSERT(!token.text.empty());
  const auto symbol = Symbol::New(token.text);
  const auto cls = Class::FindClass(symbol);
  if (!cls) {
    std::stringstream ss;
    ss << "cannot create cast, failed to find type: " << symbol;
    return ReturnError(ss, start_pos);
  }
  ASSERT(cls);
  expr::Expression* value = nullptr;
  CHECK_RESULT(ParseExpression(&value));
  ASSERT(value);
  (*result) = expr::CastExpr::New(cls, value);
  return true;
}

auto Parser::ParseInstanceOfExpr(expr::Expression** result) -> ParseResult {
  const auto start_pos = GetPos();
  const auto token = NextToken();
  EXPECT(token, Token::kInstanceOfExpr);
  ASSERT(!token.text.empty());
  const auto symbol = Symbol::New(token.text);
  const auto cls = Class::FindClass(symbol);
  if (!cls) {
    std::stringstream ss;
    ss << "cannot create instanceof expr, failed to find type: " << symbol;
    return ReturnError(ss, start_pos);
  }
  ASSERT(cls);
  expr::Expression* value = nullptr;
  CHECK_RESULT(ParseExpression(&value));
  ASSERT(value);
  (*result) = expr::InstanceOfExpr::New(cls, value);
  return true;
}

auto Parser::ParseLambda(const Token::Kind kind, Lambda** result) -> ParseResult {
  const auto lambda = Lambda::New();
  ASSERT(lambda);
  PushOwner(lambda);
  ParseScope scope(this);
  if (kind == Token::kDispatch) {
    ExpectNext(Token::kDispatch);
    SetDispatching();

    const auto local = LocalVariable::New(scope, Symbol::New("this"), lambda);
    ASSERT(local);
    LOG_IF(FATAL, !GetScope()->Add(local)) << "cannot add " << local << " to scope.";

    expr::ExpressionList body;
    LOG_IF(FATAL, !ParseExpressionList(body)) << "failed to parse expression list.";
    lambda->SetBody(body);

    std::vector<Argument*> parsed_args{};
    if (dispatched_ > 0) {
      for (auto idx = 0; idx < dispatched_; idx++) {
        const auto name = fmt::format("${}", idx);
        parsed_args.push_back(Argument::New(idx, String::New(name), false, false));
        const auto local = LocalVariable::New(scope, Symbol::New(name));
        ASSERT(local);
        LOG_IF(FATAL, !scope->Add(local)) << "failed to add " << (*local) << " to current scope.";
      }
    }

    if (!parsed_args.empty()) {  // TODO this is really dumb
      Array<Argument*>* args = Array<Argument*>::New(static_cast<word>(parsed_args.size()));
      for (const auto& arg : parsed_args) args->Push(arg);
      if (args)
        lambda->SetArgs(args);
    }
    ExpectNext(Token::kRParen);
    ClearDispatched();
    lambda->SetScope(scope);
    PopOwner();
    if (HasOwner())
      GetOwner()->AddChild(lambda);
    (*result) = lambda;
    return true;
  }

  ExpectNext(kind);
  lambda->SetScope(scope);
  CHECK_RESULT(TryParseSymbol(lambda));
  const auto local = LocalVariable::New(scope, lambda->HasSymbol() ? lambda->GetSymbol() : Symbol::New("$"), lambda);
  ASSERT(local);
  LOG_IF(FATAL, !GetScope()->Add(local)) << "cannot add " << local << " to scope.";
  // arguments
  Array<Argument*>* args = nullptr;
  CHECK_RESULT(ParseArguments(&args, true));
  if (args)
    lambda->SetArgs(args);
  // docstring
  CHECK_RESULT(TryParseDocstring(lambda));
  // body
  expr::ExpressionList body{};
  CHECK_RESULT(ParseExpressionList(body, false));
  if (body.empty() && lambda->HasDocstring())
    body.push_back(expr::LiteralExpr::New(lambda->GetDocstring()));  // TODO: should we remove the docstring
  lambda->SetBody(body);
  PopOwner();
  if (HasOwner())
    GetOwner()->AddChild(lambda);
  (*result) = lambda;
  return true;
}

static inline auto IsLiteralLong(Expression* expr) -> bool {
  if (!expr || !expr->IsLiteralExpr())
    return false;
  const auto literal = expr->AsLiteralExpr();
  ASSERT(literal);
  return literal->HasValue() && literal->GetValue()->IsLong();
}

auto Parser::ParseListExpr(expr::Expression** result) -> ParseResult {
  expr::Expression* first = nullptr;
  CHECK_RESULT(ParseExpression(&first));
  ASSERT(first);
  if (PeekEq(Token::kRange)) {
    NextToken();
    if (!IsLiteralLong(first)) {
      std::stringstream ss;
      ss << "expected " << first << " to be a literal number.";
      return ReturnError(ss, pos_);  // TODO: fix pos
    }
    expr::Expression* end = nullptr;
    CHECK_RESULT(ParseExpression(&end));
    ASSERT(end);
    if (!IsLiteralLong(end)) {
      std::stringstream ss;
      ss << "unexpected " << end << ", expected a literal number.";
      return ReturnError(ss, pos_);  // TODO: fix pos
    }
    const auto from = first->AsLiteralExpr()->GetValue()->AsLong()->Get();
    const auto to = end->AsLiteralExpr()->GetValue()->AsLong()->Get();
    (*result) = expr::LiteralExpr::New(gel::ListFromRange(from, to));
    return true;
  }
  const auto list = expr::ListExpr::New();
  list->Append(first);
  while (!PeekEq(Token::kRParen)) {
    expr::Expression* value = nullptr;
    CHECK_RESULT(ParseExpression(&value));
    ASSERT(value);
    list->Append(value);
  }
  (*result) = list;
  return true;
}

auto Parser::ParseModule(const std::string& name, Module** result) -> ParseResult {
  ParseScope scope(this);
  const auto new_module = Module::New(String::New(name), scope);
  ASSERT(new_module);
  PushOwner(new_module);
  expr::ExpressionList init_body{};
  while (!PeekEq(Token::kEndOfStream)) {
    expr::Expression* expr = nullptr;
    CHECK_RESULT(ParseExpression(&expr));
    if (expr) {
      init_body.push_back(expr);
    }
  }
  if (!init_body.empty()) {
    const auto init = new_module->CreateInitFunc(init_body);
    ASSERT(init);
    DVLOG(1000) << "created init function for " << new_module << ": " << init;
  }
  PopOwner();
  (*result) = new_module;
  return true;
}

auto Parser::ParseScript(Script** result) -> ParseResult {
  ParseScope scope(this);
  const auto script = Script::New(scope);
  ASSERT(script);
  PushOwner(script);
  while (!PeekEq(Token::kEndOfStream)) {
    const auto& peek = PeekToken();
    if (peek.IsLiteral() || peek.IsIdentifier() || peek.kind == Token::kFn || peek.kind == Token::kDispatch) {
      expr::Expression* literal = nullptr;
      CHECK_RESULT(ParseLiteralExpr(&literal));
      ASSERT(literal);
      script->Append(literal);
    } else if (peek.IsQuote()) {
      expr::Expression* quote = nullptr;
      CHECK_RESULT(ParseQuotedExpr(&quote));
      ASSERT(quote);
      script->Append(quote);
    }

    Expression* expr = nullptr;
    EXPECT_NEXT(Token::kLParen);
    const auto next = PeekToken();
    if (next.IsUnaryOp()) {
      CHECK_RESULT(ParseUnaryOpExpr(&expr));
    } else if (next.IsBinaryOp()) {
      CHECK_RESULT(ParseBinaryExpr(&expr));
    } else if (next.IsLiteral() && !next.IsIdentifier()) {
      CHECK_RESULT(ParseLiteralExpr(&expr));
    } else {
      switch (next.kind) {
        case Token::kDefNamespace: {
          LocalVariable* local = nullptr;
          CHECK_RESULT(ParseDefNamespace(&local));
          ASSERT(local && local->HasValue() && local->GetValue()->IsNamespace());
          break;
        }
        case Token::kDef: {
          CHECK_RESULT(ParseDef(&expr));
          break;
        }
        case Token::kDefn: {
          LocalVariable* local = nullptr;
          CHECK_RESULT(ParseDefn(&local));
          ASSERT(local && local->HasValue() && local->GetValue()->IsLambda());
          break;
        }
        case Token::kDefMacro: {
          LocalVariable* local = nullptr;
          CHECK_RESULT(ParseDefMacro(&local));
          ASSERT(local && local->HasValue() && local->GetValue()->IsMacro());
          break;
        }
        // Expressions
        case Token::kBeginExpr: {
          CHECK_RESULT(ParseBeginExpr(&expr));
          break;
        }
        case Token::kSet: {
          CHECK_RESULT(ParseSetExpr(&expr));
          break;
        }
        case Token::kCond: {
          CHECK_RESULT(ParseCondExpr(&expr));
          break;
        }
        case Token::kThrowExpr: {
          CHECK_RESULT(ParseThrowExpr(&expr));
          break;
        }
        case Token::kFn:
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
          CHECK_RESULT(ParseLiteralLambda(next.kind, (expr::LiteralExpr**)&expr));
          break;
        case Token::kLParen:
        case Token::kDispatch:
        case Token::kIdentifier: {
          CHECK_RESULT(ParseCallExpr(&expr));
          break;
        }
        case Token::kQuote: {
          CHECK_RESULT(ParseQuotedExpr(&expr));
          break;
        }
        case Token::kWhenExpr: {
          CHECK_RESULT(ParseWhenExpr(&expr));
          break;
        }
        case Token::kCaseExpr: {
          CHECK_RESULT(ParseCaseExpr(&expr));
          break;
        }
        case Token::kWhileExpr: {
          CHECK_RESULT(ParseWhileExpr(&expr));
          break;
        }
        case Token::kLetExpr: {
          CHECK_RESULT(ParseLetExpr(&expr));
          break;
        }
        case Token::kLetRxExpr: {
          CHECK_RESULT(ParseLetRxExpr(&expr));
          break;
        }
        case Token::kCastExpr: {
          CHECK_RESULT(ParseCastExpr(&expr));
          break;
        }
        case Token::kInstanceOfExpr: {
          CHECK_RESULT(ParseInstanceOfExpr(&expr));
          break;
        }
        case Token::kImportExpr: {
          CHECK_RESULT(ParseImportExpr(&expr));
          break;
        }
        default: {
          (*result) = nullptr;
          return UnexpectedError(NextToken());
        }
      }
    }
    EXPECT_NEXT(Token::kRParen);
    if (expr) {
      script->Append(expr);
      DVLOG(100) << "parsed: " << expr->ToString();
    }
  }
  PopOwner();
  (*result) = script;
  return true;
}

auto Parser::ParseDef(expr::Expression** result) -> ParseResult {
  EXPECT_NEXT(Token::kDef);
  Symbol* symbol = nullptr;
  CHECK_RESULT(ParseLiteralSymbol(&symbol));
  ASSERT(symbol);
  const auto scope = GetScope();
  ASSERT(scope);
  const auto local = LocalVariable::New(scope, symbol);
  ASSERT(local);
  if (!scope->Add(local)) {
    std::stringstream ss;
    ss << "failed to add " << (*local) << " to current scope.";
    return ReturnError(ss, pos_);
  }
  expr::Expression* value = nullptr;
  CHECK_RESULT(ParseExpression(&value));
  ASSERT(value);
  if (value->IsConstantExpr()) {
    local->SetValue(value->EvalToConstant(scope));
    return true;
  }
  (*result) = expr::StoreLocalExpr::New(local, value);
  return true;
}

auto Parser::ParseDefn(LocalVariable** result) -> ParseResult {
  const auto scope = GetScope();
  Lambda* lambda = nullptr;
  CHECK_RESULT(ParseLambda(Token::kDefn, &lambda));
  ASSERT(lambda && lambda->HasSymbol());
  const auto local = LocalVariable::New(scope, lambda->GetSymbol(), lambda);
  ASSERT(local);
  LOG_IF(FATAL, !scope->Add(local)) << "failed to add " << local << " to scope.";
  (*result) = local;
  return true;
}

auto Parser::ParseMacro(Macro** result) -> ParseResult {
  EXPECT_NEXT(Token::kDefMacro);
  const auto macro = Macro::New();
  ASSERT(macro);
  PushOwner(macro);
  CHECK_RESULT(TryParseSymbol(macro));
  ParseScope scope(this);
  const auto local = LocalVariable::New(scope, macro->GetSymbol() ? macro->GetSymbol() : Symbol::New("$"), macro);
  ASSERT(local);
  LOG_IF(FATAL, !GetScope()->Add(local)) << "cannot add " << local << " to scope.";
  macro->SetScope(scope);
  // arguments
  Array<Argument*>* args = nullptr;
  CHECK_RESULT(ParseArguments(&args));
  if (args)
    macro->SetArgs(args);
  // docstring
  CHECK_RESULT(TryParseDocstring(macro));
  // body
  expr::ExpressionList body{};
  CHECK_RESULT(ParseExpressionList(body, false));
  if (body.empty() && macro->HasDocstring())
    body.push_back(expr::LiteralExpr::New(macro->GetDocstring()));
  macro->SetBody(body);
  PopOwner();
  if (HasOwner())
    GetOwner()->AddChild(macro);
  (*result) = macro;
  return true;
}

auto Parser::ParseDefMacro(LocalVariable** result) -> ParseResult {
  const auto scope = GetScope();
  Macro* macro = nullptr;
  CHECK_RESULT(ParseMacro(&macro));
  ASSERT(macro);
  const auto local = LocalVariable::New(scope, macro->GetSymbol(), macro);
  ASSERT(local);
  if (!scope->Add(local)) {
    LOG(ERROR) << "failed to add " << (*local) << " to current scope.";
    (*result) = nullptr;
    return false;
  }
  (*result) = local;
  return true;
}

auto Parser::ParseNamespace(Namespace** result) -> ParseResult {
  ParseScope scope(this);
  EXPECT_NEXT(Token::kDefNamespace);
  Symbol* name = nullptr;
  CHECK_RESULT(ParseLiteralSymbol(&name));
  ASSERT(name);
  const auto ns = Namespace::New(name, scope);
  ASSERT(ns);
  PushOwner(ns);
  TryParseDocstring(ns);
  while (!PeekEq(Token::kRParen)) {
    ExpectNext(Token::kLParen);
    switch (PeekKind()) {
      case Token::kDefn: {
        LocalVariable* local = nullptr;
        CHECK_RESULT(ParseDefn(&local));
        ASSERT(local && local->HasValue() && local->GetValue()->IsLambda());
        break;
      }
      case Token::kDefMacro: {
        LocalVariable* local = nullptr;
        CHECK_RESULT(ParseDefMacro(&local));
        ASSERT(local && local->HasValue() && local->GetValue()->IsMacro());
        break;
      }
      case Token::kDefNative: {
        LocalVariable* local = nullptr;
        CHECK_RESULT(ParseDefNative(&local));
        ASSERT(local && local->HasValue() && local->GetValue()->IsNativeProcedure());
        break;
      }
      default: {
        (*result) = nullptr;
        return UnexpectedError(NextToken());
      }
    }
    ExpectNext(Token::kRParen);
  }
  PopOwner();
  if (HasOwner())
    GetOwner()->AddChild(ns);
  (*result) = ns;
  return true;
}

auto Parser::ParseDefNamespace(LocalVariable** result) -> ParseResult {
  const auto start_pos = GetPos();
  if (GetDepth() > 1)
    return UnexpectedError(NextToken());
  Namespace* ns = nullptr;
  CHECK_RESULT(ParseNamespace(&ns));
  ASSERT(ns);
  LocalVariable* local = LocalVariable::New(scope_, ns->GetSymbol(), ns);
  ASSERT(local);
  if (!scope_->Add(local)) {
    (*result) = nullptr;
    std::stringstream ss;
    ss << "failed to add " << (*local) << " to current scope.";
    return ReturnError(ss, start_pos);
  }
  (*result) = local;
  return true;
}

auto Parser::ParseDefNative(LocalVariable** local) -> ParseResult {
  ASSERT(local);
  const auto start_pos = GetPos();
  EXPECT_NEXT(Token::kDefNative);
  Symbol* symbol = nullptr;
  CHECK_RESULT(ParseLiteralSymbol(&symbol));
  ASSERT(symbol);
  const auto native = NativeProcedure::FindOrCreate(symbol);
  if (!native) {
    (*local) = nullptr;
    std::stringstream ss;
    ss << "failed to find NativeProcedure w/ Symbol: " << symbol;
    return ReturnError(ss, start_pos);
  }
  // arguments
  Array<Argument*>* args = nullptr;
  CHECK_RESULT(ParseArguments(&args));
  if (args)
    native->SetArgs(args);
  // docstring
  CHECK_RESULT(TryParseDocstring(native));
  if (!((*local) = LocalVariable::New(GetScope(), symbol, native))) {
    (*local) = nullptr;
    std::stringstream ss;
    ss << "failed to create local for: " << native;
    return ReturnError(ss, start_pos);
  }
  ASSERT((*local));
  if (!GetScope()->Add((*local))) {
    std::stringstream ss;
    ss << "failed to add " << *(*local) << " to current scope.";
    (*local) = nullptr;
    return ReturnError(ss, start_pos);
  }
  if (HasOwner())
    GetOwner()->AddChild(native);
  DVLOG(1000) << "created local " << *(*local) << " for native: " << native;
  return true;
}

#define DEF_TOKEN(Keyword, Kind) RegisterKeyword(Keyword, Kind)

void Parser::Init() {
  DEF_TOKEN("ns", Token::kDefNamespace);
  DEF_TOKEN("def", Token::kDef);
  DEF_TOKEN("defmacro", Token::kDefMacro);
  DEF_TOKEN("import", Token::kImportExpr);
  DEF_TOKEN("cons", Token::kCons);
  DEF_TOKEN("car", Token::kCar);
  DEF_TOKEN("cdr", Token::kCdr);
  DEF_TOKEN("begin", Token::kBeginExpr);
  DEF_TOKEN("add", Token::kAdd);
  DEF_TOKEN("subtract", Token::kSubtract);
  DEF_TOKEN("multiply", Token::kMultiply);
  DEF_TOKEN("divide", Token::kDivide);
  DEF_TOKEN("fn", Token::kFn);
  DEF_TOKEN("quote", Token::kQuote);
  DEF_TOKEN("not", Token::kNot);
  DEF_TOKEN("and", Token::kBinaryAnd);
  DEF_TOKEN("or", Token::kBinaryOr);
  DEF_TOKEN("throw", Token::kThrowExpr);
  DEF_TOKEN("eq?", Token::kEquals);
  DEF_TOKEN("instanceof?", Token::kInstanceOf);
  DEF_TOKEN("nonnull?", Token::kNonnull);
  DEF_TOKEN("null?", Token::kNull);
  DEF_TOKEN("set!", Token::kSet);
  DEF_TOKEN("cond", Token::kCond);
  DEF_TOKEN("when", Token::kWhenExpr);
  DEF_TOKEN("case", Token::kCaseExpr);
  DEF_TOKEN("while", Token::kWhileExpr);
  DEF_TOKEN("defn", Token::kDefn);
  DEF_TOKEN("let", Token::kLetExpr);
  DEF_TOKEN("let:rx", Token::kLetRxExpr);
  DEF_TOKEN("defnative", Token::kDefNative);
}
}  // namespace gel