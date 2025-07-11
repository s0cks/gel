#include "gel/parser.h"

#include <cctype>
#include <cstdlib>
#include <fmt/format.h>
#include <glog/logging.h>
#include <optional>

#include "gel/argument.h"
#include "gel/common.h"
#include "gel/expr/expression.h"
#include "gel/expr/exprs.h"
#include "gel/expr/seq_expr.h"
#include "gel/lambda.h"
#include "gel/local.h"
#include "gel/local_scope.h"
#include "gel/module.h"
#include "gel/module_loader.h"
#include "gel/namespace.h"
#include "gel/procedure.h"
#include "gel/script.h"
#include "gel/token.h"
#include "gel/tracing.h"
#include "gel/type_traits.h"
#include "gel/types.h"

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

class TopLevelScope {
  DEFINE_NON_COPYABLE_TYPE(TopLevelScope);

 private:
  Parser* parser_;

 public:
  TopLevelScope(Parser* parser, Module* value) :
    parser_(parser) {
    ASSERT(parser_);
    parser_->PushTopLevel(value);
  }
  TopLevelScope(Parser* parser, Script* value) :
    parser_(parser) {
    ASSERT(parser_);
    parser_->PushTopLevel(value);
  }
  TopLevelScope(Parser* parser, Namespace* value) :
    parser_(parser) {
    ASSERT(parser_);
    parser_->PushTopLevel(value);
  }
  TopLevelScope(Parser* parser, Lambda* value) :
    parser_(parser) {
    ASSERT(parser_);
    parser_->PushTopLevel(value);
  }
  TopLevelScope(Parser* parser, Class* value) :
    parser_(parser) {
    ASSERT(parser_);
    parser_->PushTopLevel(value);
  }
  TopLevelScope(Parser* parser, Macro* value) :
    parser_(parser) {
    ASSERT(parser_);
    parser_->PushTopLevel(value);
  }
  ~TopLevelScope() {
    ASSERT(parser_);
    parser_->PopTopLevel();
  }
};

void Parser::PushTopLevel(Script* rhs) {
  ASSERT(rhs);
  toplevel_ = rhs;
}

void Parser::PushTopLevel(Module* rhs) {
  ASSERT(rhs);
  toplevel_ = rhs;
}

void Parser::PushTopLevel(Class* rhs) {
  ASSERT(rhs);
  toplevel_ = rhs;
}

void Parser::PushTopLevel(Namespace* rhs) {
  ASSERT(rhs);
  if (HasTopLevel()) {
    ASSERT(GetTopLevel()->IsModule() || GetTopLevel()->IsScript());
    rhs->SetOwner(GetTopLevel());
    GetTopLevel()->AddChild(rhs);
  }
  toplevel_ = rhs;
}

void Parser::PushTopLevel(Macro* rhs) {
  ASSERT(rhs);
  if (HasTopLevel()) {
    const auto parent = GetTopLevel()->IsModule() ? GetTopLevel()->AsModule()->GetDefaultNamespace() : GetTopLevel();
    rhs->SetOwner(parent);
    parent->AddChild(rhs);
  }
  toplevel_ = rhs;
}

void Parser::PushTopLevel(Lambda* rhs) {
  ASSERT(rhs);
  if (HasTopLevel()) {
    const auto parent = GetTopLevel()->IsModule() ? GetTopLevel()->AsModule()->GetDefaultNamespace() : GetTopLevel();
    rhs->SetOwner(parent);
    parent->AddChild(rhs);
  }
  toplevel_ = rhs;
}

void Parser::PopTopLevel() {
  if (!HasTopLevel())
    return;
  if (GetTopLevel()->IsModule() || GetTopLevel()->IsScript() || GetTopLevel()->IsClass()) {
    toplevel_ = nullptr;
  } else if (GetTopLevel()->IsNamespace()) {
    toplevel_ = GetTopLevel()->AsNamespace()->GetOwner();
  } else if (GetTopLevel()->IsMacro()) {
    toplevel_ = GetTopLevel()->AsMacro()->GetOwner();
  } else if (GetTopLevel()->IsLambda()) {
    toplevel_ = GetTopLevel()->AsLambda()->GetOwner();
  }
}

template <HasDocstring T>
auto Parser::TryParseDocstring(T* owner) -> ParseResult {
  ASSERT(owner);
  if (PeekEq(Token::kLiteralString)) {
    String* docstring = nullptr;
    CHECK_RESULT(ParseLiteralString(&docstring));
    ASSERT(docstring);
    owner->SetDocstring(docstring);
  }
  return true;
}

template auto Parser::TryParseDocstring(Macro*) -> ParseResult;
template auto Parser::TryParseDocstring(Lambda*) -> ParseResult;
template auto Parser::TryParseDocstring(Namespace*) -> ParseResult;

template <WithSymbol T>
auto Parser::TryParseSymbol(T* owner) -> ParseResult
  requires(HasMutableSymbol<T>)
{
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
  if (HasTopLevel() && GetTopLevel()->IsNamespace()) {
    (*result) = GetTopLevel()->AsNamespace()->CreateSymbol(next.text);
  } else if (HasTopLevel() && GetTopLevel()->IsClass()) {
    (*result) = GetTopLevel()->AsClass()->CreateSymbol(next.text);
  } else if (HasTopLevel() && GetTopLevel()->IsModule()) {
    (*result) = GetTopLevel()->AsModule()->GetDefaultNamespace()->CreateSymbol(next.text);
  } else {
    const auto symbol = Symbol::New(next.text);
    ASSERT(symbol);
    (*result) = symbol;
  }
  ASSERT((*result));
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
  SetParsingLiteralMap();

  Symbol* key = nullptr;
  expr::Expression* value = nullptr;

  expr::NewMapExpr::EntryList data{};
  while (!PeekEq(Token::kRBrace)) {
    CHECK_RESULT(ParseLiteralSymbol(&key));
    ASSERT(key);

    if (PeekEq(Token::kColon))
      NextToken();

    CHECK_RESULT(ParseExpression(&value));
    ASSERT(value);
    if (PeekEq(Token::kComma))
      NextToken();
    data.emplace_back(key, value);
  }
  EXPECT_NEXT(Token::kRBrace);
  ClearParsingLiteralMap();
  (*result) = expr::NewMapExpr::New(data);
  ASSERT((*result));
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

auto Parser::ParseLiteralVec(expr::Expression** result) -> ParseResult {
  EXPECT_NEXT(Token::kLBracket);

  expr::Expression* value = nullptr;
  expr::ExpressionList values{};
  do {
    CHECK_RESULT(ParseExpression(&value));
    ASSERT(value);
    values.push_back(value);
    if (PeekEq(Token::kRBracket)) {
      if (values.size() >= 2)
        break;
      return UnexpectedError(NextToken());
    }
  } while (values.size() < 3);
  EXPECT_NEXT(Token::kRBracket);

  if (values.size() == 2) {
    (*result) = expr::NewExpr::New(Vec2::GetClass(), values);
    return true;
  } else if (values.size() == 3) {
    (*result) = expr::NewExpr::New(Vec3::GetClass(), values);
    return true;
  }
  NOT_IMPLEMENTED(FATAL);
}

auto Parser::ParseLiteralSet(expr::Expression** result) -> ParseResult {
  EXPECT_NEXT(Token::kBeginSet);

  expr::Expression* value = nullptr;
  expr::ExpressionList values{};
  do {
    CHECK_RESULT(ParseExpression(&value));
    values.push_back(value);
    if (PeekEq(Token::kRBrace))
      break;
  } while (true);
  EXPECT_NEXT(Token::kRBrace);

  (*result) = expr::NewExpr::New(Set::GetClass(), values);
  return true;
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
    case Token::kLiteralNil:
      NextToken();
      (*result) = Nil::Get();
      return true;
    default:
      return UnexpectedError(NextToken());
  }
}

auto Parser::ParseLiteralExpr(expr::Expression** result) -> ParseResult {
  if (PeekEq(Token::kFn) || PeekEq(Token::kDispatch)) {
    return ParseLiteralLambda(PeekKind(),
                              (expr::LiteralExpr**)result);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  } else if (PeekEq(Token::kLBrace)) {
    return ParseMap(result);
  } else if (PeekEq(Token::kLBracket)) {
    return ParseLiteralVec(result);
  } else if (PeekEq(Token::kBeginSet)) {
    return ParseLiteralSet(result);
  }

  Object* literal = nullptr;
  CHECK_RESULT(ParseLiteralValue(&literal));
  ASSERT(literal);
  (*result) = expr::LiteralExpr::New(literal);
  return true;
}

auto Parser::ParseDoExpr(expr::Expression** result) -> ParseResult {
  // ParseScope scope(this);
  EXPECT_NEXT(Token::kDoExpr);
  expr::SeqExpr* body = nullptr;
  ParseScope scope(this);
  CHECK_RESULT(ParseSeqExpr(&body));
  (*result) = expr::DoExpr::New(body);
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
        ASSERT(local);
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

        const auto field = type->FindField(symbol->GetSymbolName());
        if (field) {
          (*result) = expr::LoadFieldExpr::New(expr::LiteralExpr::New(local->GetSymbol()), field);
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
    const auto clause = expr::ClauseExpr::New(a, b);
    ASSERT(clause);
    clauses.push_back(clause);
  } while (!PeekEq(Token::kRParen));
  (*result) = CondExpr::New(clauses, alt);
  return true;
}

auto Parser::ParseLetExpr(expr::Expression** result) -> ParseResult {
  const auto start_pos = GetPos();
  EXPECT_NEXT(Token::kLetExpr);
  expr::BindingList bindings{};
  expr::SeqExpr* body = nullptr;
  ParseScope let_scope(this);
  if (!ParseBindingList(bindings))
    return ReturnError("failed to parse let-expr bindings", start_pos);
  CHECK_RESULT(ParseSeqExpr(&body));
  (*result) = LetExpr::New(let_scope, bindings, body);
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

auto Parser::ParseBinding(expr::BindingExpr** result) -> ParseResult {
  Symbol* symbol = nullptr;
  CHECK_RESULT(ParseLiteralSymbol(&symbol));
  const auto scope = GetScope();
  LocalVariable* local = LocalVariable::New(scope, symbol);
  LOG_IF(FATAL, !scope->Add(local)) << "failed to add " << (*local) << " to current scope.";
  expr::Expression* value = nullptr;
  CHECK_RESULT(ParseExpression(&value));
  (*result) = expr::BindingExpr::New(local, value);
  return true;
}

auto Parser::ParseBindingList(expr::BindingList& bindings, const bool push_scope) -> ParseResult {
  if (push_scope)
    PushScope();

  EXPECT_NEXT(Token::kLBracket);
  expr::BindingExpr* binding = nullptr;
  while (!PeekEq(Token::kRBracket)) {
    CHECK_RESULT(ParseBinding(&binding));
    if (binding)
      bindings.push_back(binding);
  }
  EXPECT_NEXT(Token::kRBracket);

  if (push_scope)
    PopScope();
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
    } else if (next.kind == Token::kLBracket) {
      CHECK_RESULT(ParseLiteralVec(result));
      return true;
    } else if (next.kind == Token::kLBrace) {
      CHECK_RESULT(ParseMap(result));
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
      case Token::kDefType: {
        LocalVariable* local = nullptr;
        CHECK_RESULT(ParseDefType(&local));
        ASSERT(local && local->HasValue() && local->GetValue()->IsClass());
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
      case Token::kDoExpr: {
        CHECK_RESULT(ParseDoExpr(result));
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
      case Token::kWhileExpr: {
        CHECK_RESULT(ParseWhileExpr(result));
        break;
      }
      case Token::kForeachExpr: {
        CHECK_RESULT(ParseForeachExpr(result));
        break;
      }
      case Token::kLetExpr: {
        CHECK_RESULT(ParseLetExpr(result));
        break;
      }
      case Token::kImportExpr: {
        CHECK_RESULT(ParseImportExpr(result));
        break;
      }
      case Token::kSetFirst:
      case Token::kSetSecond: {
        CHECK_RESULT(ParseSetPairField(NextToken(), result));
        break;
      }
      default:
        return UnexpectedError(next);
    }
  }
  EXPECT_NEXT(Token::kRParen);
  return true;
}

auto Parser::ParseImportExpr(expr::Expression** result) -> ParseResult {
  const auto start_pos = GetPos();
  EXPECT_NEXT(Token::kImportExpr);
  const auto& next = NextToken();
  EXPECT(next, Token::kLiteralString);
  const auto module_path = next.text;
  const auto target_module = GetModuleLoader()->LoadModule(module_path);
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

auto Parser::ParseWhileExpr(expr::Expression** result) -> ParseResult {
  EXPECT_NEXT(Token::kWhileExpr);
  expr::Expression* test = nullptr;
  CHECK_RESULT(ParseExpression(&test));
  ASSERT(test);
  expr::SeqExpr* body = nullptr;
  CHECK_RESULT(ParseSeqExpr(&body));
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
      return NextToken(Token::kEq);
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
        case '{':
          Advance(2);
          return NextToken(Token::kBeginSet);
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
      Advance();
      return NextToken(Token::kColon);
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
    return whole ? NextToken(Token::kLiteralLong, GetBufferedText())
                 : NextToken(Token::kLiteralDouble, GetBufferedText());
  } else if (IsValidIdentifierChar(next, true)) {
    token_len_ = 0;
    auto ckw = keywords_;
    while (IsValidIdentifierChar(PeekChar(), token_len_ == 0)) {
      if (PeekChar() == '?') {
        if (IsParsingArgs()) {
          break;
        }
      } else if ((PeekChar() == '.' && PeekChar(1) == '.') || (PeekChar() == ':' && IsParsingLiteralMap())) {
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

auto Parser::ParseLambda(const Token::Kind kind, Lambda** result) -> ParseResult {
  const auto lambda = Lambda::New();
  ASSERT(lambda);
  ParseScope scope(this);
  lambda->SetScope(scope);
  TopLevelScope toplevel(this, lambda);
  if (kind == Token::kDispatch) {
    ExpectNext(Token::kDispatch);
    SetDispatching();

    const auto local = LocalVariable::New(scope, Symbol::New("this"), lambda);
    ASSERT(local);
    LOG_IF(FATAL, !GetScope()->Add(local)) << "cannot add " << local << " to scope.";

    expr::ExpressionList body;
    LOG_IF(FATAL, !ParseExpressionList(body)) << "failed to parse expression list.";
    if (!body.empty())
      lambda->SetBody(expr::SeqExpr::New(body));

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
      for (const auto& arg : parsed_args)
        args->Push(arg);
      if (args)
        lambda->SetArgs(args);
    }
    ExpectNext(Token::kRParen);
    ClearDispatched();
    (*result) = lambda;
    return true;
  }

  ExpectNext(kind);
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
  expr::SeqExpr* body = nullptr;
  CHECK_RESULT(ParseSeqExpr(&body));
  if (body)
    lambda->SetBody(body);

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
  } else if (PeekEq(Token::kDot)) {
    NextToken();
    expr::Expression* second = nullptr;
    CHECK_RESULT(ParseExpression(&second));
    ASSERT(second);
    (*result) = expr::BinaryOpExpr::New(BinaryOp::kCons, first, second);
    return true;
  }
  const auto list = expr::SeqExpr::New();
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
  const auto new_module = Module::New(Symbol::New(name), scope);
  ASSERT(new_module);
  TopLevelScope toplevel(this, new_module);
  scope->AddThisValue(new_module);
  expr::SeqExpr* init_body = expr::SeqExpr::New();
  while (!PeekEq(Token::kEndOfStream)) {
    expr::Expression* expr = nullptr;
    CHECK_RESULT(ParseExpression(&expr));
    if (expr) {
      init_body->Append(expr);
    }
  }
  if (!init_body->IsEmpty()) {
    const auto init = Module::CreateConstructor(new_module, init_body);
    ASSERT(init);
    new_module->SetInit(init);
    DVLOG(1000) << "created init function for " << new_module << ": " << init;
  }
  (*result) = new_module;
  return true;
}

auto Parser::ParseScript(Script** result) -> ParseResult {
  ParseScope scope(this);
  const auto script = Script::New(scope);
  ASSERT(script);
  scope->AddThisValue(script);
  TopLevelScope toplevel(this, script);
  expr::SeqExpr* body = nullptr;
  CHECK_RESULT(ParseSeqExpr(&body, Token::kEndOfStream));
  if (body)
    script->SetBody(body);
  (*result) = script;
  return true;
}

auto Parser::ParseSetPairField(const Token& token, expr::Expression** result) -> ParseResult {
  Field* field = nullptr;
  switch (token.kind) {
    case Token::kSetFirst: {
      field = Pair::kFirstField;
      break;
    }
    case Token::kSetSecond: {
      field = Pair::kSecondField;
      break;
    }
    default:
      TokenKindBitSet expected{};
      expected.set(Token::kSetFirst);
      expected.set(Token::kSetSecond);
      return UnexpectedError(token, expected);
  }
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

auto Parser::ParseDef(expr::Expression** result) -> ParseResult {
  DLOG(INFO) << "parsing def-expr....";
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
  (*result) = expr::StoreLocalExpr::New(local, value);
  return true;
}

auto Parser::ParseDefType(LocalVariable** result) -> ParseResult {
  const auto scope = GetScope();
  ASSERT(scope);

  const auto start_pos = GetPos();

  EXPECT_NEXT(Token::kDefType);
  Symbol* symbol = nullptr;
  CHECK_RESULT(ParseLiteralSymbol(&symbol));
  ASSERT(symbol);
  const auto cls = Class::FindClass(symbol);
  if (!cls) {
    std::stringstream ss;
    ss << "cannot find type: " << symbol;
    return NewParseError(ss.str(), start_pos);
  }
  TopLevelScope toplevel(this, cls);

  expr::SeqExpr* init = nullptr;
  CHECK_RESULT(ParseSeqExpr(&init));

  if (!PeekEq(Token::kRParen))
    return Unexpected(Token::kRParen, NextToken());
  const auto local = LocalVariable::New(scope, symbol, cls);
  ASSERT(local);
  (*result) = local;
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
  CHECK_RESULT(TryParseSymbol(macro));
  ParseScope scope(this);
  TopLevelScope toplevel(this, macro);
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

auto Parser::ParseSeqExpr(expr::SeqExpr** result, const bool allow_empty, const Token::Kind end) -> ParseResult {
  const auto start_pos = GetPos();
  expr::SeqExpr* seq = expr::SeqExpr::New();
  while (!PeekEq(end)) {
    expr::Expression* expr = nullptr;
    CHECK_RESULT(ParseExpression(&expr));
    if (expr)
      seq->Append(expr);
  }
  if (seq->IsEmpty() && !allow_empty)
    return ReturnError("", start_pos);  // @s0cks TODO: add message
  (*result) = seq;
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
  TopLevelScope toplevel(this, ns);
  scope->AddThisValue(ns);
  TryParseDocstring(ns);
  expr::SeqExpr* body = nullptr;
  CHECK_RESULT(ParseSeqExpr(&body));
  if (body && !body->IsEmpty()) {
    const auto init = Namespace::CreateConstructor(ns, body);
    ASSERT(init);
    ns->SetInit(init);
  }
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

auto Parser::ParseForeachExpr(expr::Expression** result) -> ParseResult {
  EXPECT_NEXT(Token::kForeachExpr);

  expr::BindingList bindings{};
  CHECK_RESULT(ParseBindingList(bindings));

  expr::SeqExpr* body = nullptr;
  CHECK_RESULT(ParseSeqExpr(&body));
  (*result) = expr::ForeachExpr::New(bindings, body);
  return true;
}

auto Parser::ParseDefNative(LocalVariable** local) -> ParseResult {
  ASSERT(local);
  const auto start_pos = GetPos();
  EXPECT_NEXT(Token::kDefNative);
  Symbol* symbol = nullptr;
  CHECK_RESULT(ParseLiteralSymbol(&symbol));
  ASSERT(symbol);

  const auto native = HasTopLevel() && GetTopLevel()->IsClass()
                        ? GetTopLevel()->AsClass()->FindOrCreateNativeProcedure(symbol)
                        : NativeProcedure::FindOrCreate(symbol);
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
  DEF_TOKEN("do", Token::kDoExpr);
  DEF_TOKEN("add", Token::kAdd);
  DEF_TOKEN("subtract", Token::kSubtract);
  DEF_TOKEN("multiply", Token::kMultiply);
  DEF_TOKEN("divide", Token::kDivide);
  DEF_TOKEN("fn", Token::kFn);
  DEF_TOKEN("quote", Token::kQuote);
  DEF_TOKEN("not", Token::kNot);
  DEF_TOKEN("bit-and", Token::kBitAnd);
  DEF_TOKEN("bit-or", Token::kBitOr);
  DEF_TOKEN("bit-xor", Token::kBitXor);
  DEF_TOKEN("bit-shl", Token::kShiftLeft);
  DEF_TOKEN("bit-shr", Token::kShiftRight);
  DEF_TOKEN("bit-not", Token::kBitNot);
  DEF_TOKEN("throw", Token::kThrowExpr);
  DEF_TOKEN("eq?", Token::kEq);
  DEF_TOKEN("instanceof?", Token::kInstanceOf);
  DEF_TOKEN("nonnull?", Token::kNonnull);
  DEF_TOKEN("null?", Token::kNull);
  DEF_TOKEN("set!", Token::kSet);
  DEF_TOKEN("set-first!", Token::kSetFirst);
  DEF_TOKEN("set-second!", Token::kSetSecond);
  DEF_TOKEN("cond", Token::kCond);
  DEF_TOKEN("while", Token::kWhileExpr);
  DEF_TOKEN("defn", Token::kDefn);
  DEF_TOKEN("let", Token::kLetExpr);
  DEF_TOKEN("defnative", Token::kDefNative);
  DEF_TOKEN("deftype", Token::kDefType);
  DEF_TOKEN("foreach", Token::kForeachExpr);
  DEF_TOKEN("true", Token::kLiteralTrue);
  DEF_TOKEN("false", Token::kLiteralFalse);
  DEF_TOKEN("nil", Token::kLiteralNil);
}
}  // namespace gel