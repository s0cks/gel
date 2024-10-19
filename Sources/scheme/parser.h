#ifndef SCM_PARSER_H
#define SCM_PARSER_H

#include <glog/logging.h>

#include <istream>
#include <ostream>
#include <utility>

#include "scheme/ast.h"
#include "scheme/lexer.h"

namespace scm {
class Parser {
  DEFINE_NON_COPYABLE_TYPE(Parser);

 private:
  TokenStream& stream_;

 protected:
  inline auto stream() const -> TokenStream& {
    return stream_;
  }

  auto ParseIdentifier() -> std::string;

  auto ParseForm() -> ast::Form*;
  auto ParseLiteral() -> ast::ConstantExpr*;

  auto ParseVariableDefinition() -> ast::VariableDef*;
  auto ParseVariable() -> Variable*;
  auto ParseVariableList(VariableList& variables) -> bool;

  auto ParseBeginDefinition() -> ast::BeginDef*;
  auto ParseCallProcExpr() -> ast::CallProcExpr*;

  auto ParseBinaryOp() -> ast::BinaryOp;
  auto ParseBinaryOpExpr() -> ast::BinaryOpExpr*;

  auto ParseExpression() -> ast::Expression*;
  auto ParseExpressionList() -> ast::ExpressionList*;

  auto ParseDefinition() -> ast::Definition*;
  auto ParseDefinitionList(ast::DefinitionList& definitions) -> bool;

  inline auto PeekEq(const Token::Kind rhs) const -> bool {
    const auto& peek = stream().Peek();
    return peek.kind == rhs;
  }

  inline auto ExpectNext(const Token::Kind rhs) -> bool {
    const auto& peek = stream().Peek();
    if (peek.kind == rhs)
      return true;
    LOG(FATAL) << "unexpected " << stream().Next() << " expected: " << rhs;
    return false;
  }

 public:
  explicit Parser(TokenStream& stream) :
    stream_(stream) {}
  ~Parser() = default;
  auto ParseProgram() -> ast::Program*;

 public:
  auto Parse(const uint8_t* data, const uint64_t length) -> ast::Program*;
};
}  // namespace scm

#endif  // SCM_PARSER_H
