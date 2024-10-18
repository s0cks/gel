#include "scheme/parser.h"

#include <glog/logging.h>

#include <cctype>

#include "scheme/ast.h"
#include "scheme/common.h"

namespace scm {
auto Parser::ParseLiteral() -> ast::ConstantExpr* {
  const auto& next = stream().Next();
  DLOG(INFO) << "next: " << next;
  switch (next.kind) {
    case Token::kLiteralTrue:
      return ast::ConstantExpr::New(Bool::True());
    case Token::kLiteralFalse:
      return ast::ConstantExpr::New(Bool::False());
    case Token::kLiteralNumber:
    case Token::kLiteralLong:
      return ast::ConstantExpr::New(Number::New(next.AsLong()));
    case Token::kLiteralDouble:
      return ast::ConstantExpr::New(Number::New(static_cast<uint64_t>(next.AsDouble())));
    default:
      LOG(ERROR) << "unexpected token: " << next;
      return nullptr;
  }
}

auto Parser::ParseIdentifier() -> std::string {
  if (!ExpectNext(Token::kIdentifier))
    return nullptr;
  const auto& next = stream().Next();
  ASSERT(next.kind == Token::kIdentifier);
  ASSERT(!next.text.empty());
  return next.text;
}

auto Parser::ParseVariable() -> Variable* {
  const auto ident = ParseIdentifier();
  return new Variable(ident);
}

auto Parser::ParseExpression() -> ast::Expression* {
  const auto& peek = stream().Peek();
  if (peek.IsLiteral())
    return ParseLiteral();
  switch (peek.kind) {
    case Token::kIdentifier: {
      const auto ident = ParseIdentifier();
      return ast::LoadVariableExpr::New(new Variable(ident));
    }
    case Token::kLParen:
      stream().Next();
      return ParseCallProcExpr();
    default:
      LOG(FATAL) << "unexpected: " << peek;
      return nullptr;
  }
}

auto Parser::ParseVariableDefinition() -> ast::VariableDef* {
  if (!ExpectNext(Token::kVariableDef))
    return nullptr;
  stream().Next();
  const auto var = ParseVariable();
  ASSERT(var);
  DLOG(INFO) << "var: " << var->ToString();
  const auto expr = ParseExpression();
  ASSERT(expr);
  return ast::VariableDef::New(var, ast::Value::New(expr));
}

auto Parser::ParseDefinition() -> ast::Definition* {
  if (!ExpectNext(Token::kLParen))
    return nullptr;
  stream().Next();

  ast::Definition* defn = nullptr;
  const auto& peek = stream().Peek();
  switch (peek.kind) {
    case Token::kBeginDef:
      defn = ParseBeginDefinition();
      break;
    case Token::kVariableDef:
      defn = ParseVariableDefinition();
      break;
    default:
      LOG(FATAL) << "unexpected: " << peek;
      return nullptr;
  }

  if (!ExpectNext(Token::kRParen))
    return nullptr;
  stream().Next();
  return defn;
}

auto Parser::ParseDefinitionList(ast::DefinitionList& definitions) -> bool {
  while (!PeekEq(Token::kRParen)) {  // TODO: cleanup
    const auto defn = ParseDefinition();
    ASSERT(defn);
    definitions.push_back(defn);
  }
  return true;
}

auto Parser::ParseBeginDefinition() -> ast::BeginDef* {
  if (!ExpectNext(Token::kBeginDef))
    return nullptr;
  stream().Next();
  ast::DefinitionList definitions;
  if (!ParseDefinitionList(definitions))
    return nullptr;
  return ast::BeginDef::New(definitions);
}

auto Parser::ParseExpressionList() -> ast::ExpressionList* {
  const auto values = ast::ExpressionList::New();
  do {
    const auto& peek = stream().Peek();
    switch (peek.kind) {
      case Token::kRParen:
        return values;
      default:
        values->Append(ParseExpression());
        continue;
    }
  } while (true);
}

auto Parser::ParseCallProcExpr() -> ast::CallProcExpr* {
  const auto ident = ParseIdentifier();
  const auto args = ParseExpressionList();
  ASSERT(args);
  return ast::CallProcExpr::New(ident, args);
}

auto Parser::ParseBinaryOpExpr() -> ast::BinaryOpExpr* {
  const auto& next = stream().Next();
  switch (next.kind) {
    case Token::kPlus: {
      const auto left = ast::Value::New(ParseExpression());
      const auto right = ast::Value::New(ParseExpression());
      return ast::BinaryOpExpr::New(ast::kAddOp, left, right);
    }
    case Token::kMinus:
    case Token::kDivide:
    default:
      LOG(FATAL) << "unexpected: " << next;
      return nullptr;
  }
}

auto Parser::ParseForm() -> ast::Form* {
  if (!ExpectNext(Token::kLParen))
    return nullptr;
  stream().Next();

  ast::Form* form = nullptr;
  const auto& next = stream().Peek();
  DLOG(INFO) << "peek: " << next;
  switch (next.kind) {
    case Token::kVariableDef:
      form = ParseVariableDefinition();
      break;
    case Token::kBeginDef:
      form = ParseBeginDefinition();
      break;
    case Token::kIdentifier: {
      form = ParseCallProcExpr();
      break;
    }
    case Token::kPlus:
    case Token::kMinus:
      form = ParseBinaryOpExpr();
      break;
    default:
      LOG(FATAL) << "unexpected token: " << stream().Next();
      return nullptr;
  }
  ASSERT(form);

  if (!ExpectNext(Token::kRParen))
    return nullptr;
  stream().Next();
  return form;
}

auto Parser::ParseProgram() -> ast::Program* {
  const auto program = ast::Program::New();
  ASSERT(program);
  while (true) {
    const auto& peek = stream().Peek();
    DLOG(INFO) << "peek: " << peek;
    if (peek.IsLiteral()) {
      program->Append(ParseLiteral());
      continue;
    }

    switch (peek.kind) {
      case Token::kLParen:
        program->Append(ParseForm());
      case Token::kEndOfStream:
        return program;
      default:
        LOG(ERROR) << "unexpected token: " << stream().Next();
        break;
    }
  }
}

auto Parser::Parse(const uint8_t* data, const uint64_t length) -> ast::Program* {
  ASSERT(data);
  ASSERT(length >= 0 && length <= TokenStream::kChunkSize);
  ByteTokenStream stream(data, length);
  Parser parser(stream);
  return parser.ParseProgram();
}
}  // namespace scm