#ifndef SCM_PARSER_H
#define SCM_PARSER_H

#include <glog/logging.h>

#include <istream>
#include <ostream>
#include <utility>

#include "scheme/common.h"
#include "scheme/expression.h"
#include "scheme/instruction.h"
#include "scheme/lambda.h"
#include "scheme/lexer.h"
#include "scheme/module.h"
#include "scheme/program.h"

namespace scm {
class Parser {
  DEFINE_NON_COPYABLE_TYPE(Parser);

 private:
  TokenStream& stream_;
  LocalScope* scope_;

 protected:
  inline auto stream() const -> TokenStream& {
    return stream_;
  }

  inline void SetScope(LocalScope* scope) {
    ASSERT(scope);
    scope_ = scope;
  }

  inline auto GetScope() const -> LocalScope* {
    return scope_;
  }

  // Definitions
  template <const bool IsTopLevel = false>
  static inline auto IsValidDefinition(const Token& rhs) -> bool {
    switch (rhs.kind) {
      case Token::kModuleDef:
        return IsTopLevel;
      case Token::kLocalDef:
        return true;
      default:
        return false;
    }
  }

  template <const google::LogSeverity Severity = google::ERROR>
  inline auto Unexpected(const Token::Kind expected, const Token& actual) -> bool {
    ASSERT(actual.kind != expected);
    LOG_AT_LEVEL(Severity) << "unexpected: " << actual << ", expected: " << expected;
    return false;
  }

  inline auto PeekEq(const Token::Kind rhs) const -> bool {
    const auto& peek = stream().Peek();
    return peek.kind == rhs;
  }

  inline void ExpectNext(const Token::Kind rhs) {
    const auto& next = stream().Next();
    LOG_IF(FATAL, next.kind != rhs) << "unexpected: " << next << ", expected: " << rhs;
  }

  // Misc
  void PushScope();
  void PopScope();
  auto ParseSymbol() -> Symbol*;
  auto ParseCondExpr() -> CondExpr*;
  auto ParseConsExpr() -> ConsExpr*;
  auto ParseLoadSymbol() -> LoadVariableInstr*;
  auto ParseArguments(ArgumentSet& args) -> bool;
  auto ParseSymbolList(SymbolList& symbols) -> bool;
  auto ParseIdentifier(std::string& result) -> bool;

  // Expressions
  auto ParseSetExpr() -> SetExpr*;
  auto ParseCallProcExpr() -> CallProcExpr*;
  auto ParseLiteralExpr() -> LiteralExpr*;
  auto ParseQuoteExpr() -> LiteralExpr*;
  auto ParseBeginExpr() -> BeginExpr*;
  auto ParseUnaryExpr() -> expr::UnaryExpr*;
  auto ParseBinaryOpExpr() -> BinaryOpExpr*;
  auto ParseLambdaExpr() -> LambdaExpr*;
  auto ParseThrowExpr() -> ThrowExpr*;

  // Definitions
  auto ParseDefinition() -> expr::Definition*;
  auto ParseLocalDef() -> expr::LocalDef*;
  auto ParseImportDef() -> expr::ImportDef*;
  auto ParseMacroDef() -> expr::MacroDef*;

 public:
  explicit Parser(TokenStream& stream, LocalScope* scope = LocalScope::New()) :
    stream_(stream),
    scope_(scope) {
    ASSERT(scope_);
  }
  ~Parser() = default;

  auto ParseModuleDef() -> expr::ModuleDef*;
  auto ParseExpression() -> Expression*;

  auto ParseProgram() -> Program*;
  auto Parse(const uint8_t* data, const uint64_t length) -> Program*;

 public:
  static inline auto Parse(TokenStream& stream) -> Program* {
    Parser parser(stream);
    return parser.ParseProgram();
  }

  static inline auto Parse(const std::string& expr) -> Program* {
    ByteTokenStream stream(expr);
    return Parse(stream);
  }

  static inline auto ParseModule(TokenStream& stream) -> expr::ModuleDef* {
    Parser parser(stream);
    return parser.ParseModuleDef();
  }

  static inline auto ParseModule(const std::string& expr) -> expr::ModuleDef* {
    ASSERT(!expr.empty());
    ByteTokenStream stream(expr);
    return ParseModule(stream);
  }
};
}  // namespace scm

#endif  // SCM_PARSER_H
