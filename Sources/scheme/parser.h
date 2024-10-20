#ifndef SCM_PARSER_H
#define SCM_PARSER_H

#include <glog/logging.h>

#include <istream>
#include <ostream>
#include <utility>

#include "scheme/expression.h"
#include "scheme/lexer.h"
#include "scheme/program.h"

namespace scm {
class Parser {
  DEFINE_NON_COPYABLE_TYPE(Parser);

 private:
  TokenStream& stream_;

 protected:
  inline auto stream() const -> TokenStream& {
    return stream_;
  }

  auto ParseSymbol() -> Symbol*;
  auto ParseLiteralExpr() -> LiteralExpr*;
  auto ParseBeginExpr() -> BeginExpr*;
  auto ParseBinaryOpExpr() -> BinaryOpExpr*;
  auto ParseDefineExpr() -> DefineExpr*;
  auto ParseExpression() -> Expression*;

  inline auto PeekEq(const Token::Kind rhs) const -> bool {
    const auto& peek = stream().Peek();
    return peek.kind == rhs;
  }

  inline void ExpectNext(const Token::Kind rhs) {
    const auto& next = stream().Next();
    LOG_IF(FATAL, next.kind != rhs) << "unexpected: " << next << ", expected: " << rhs;
  }

 public:
  explicit Parser(TokenStream& stream) :
    stream_(stream) {}
  ~Parser() = default;
  auto ParseProgram() -> Program*;

 public:
  auto Parse(const uint8_t* data, const uint64_t length) -> Program*;

  static inline auto Parse(TokenStream& stream) -> Program* {
    Parser parser(stream);
    return parser.ParseProgram();
  }

  static inline auto Parse(const std::string& expr) -> Program* {
    ByteTokenStream stream(expr);
    return Parse(stream);
  }
};
}  // namespace scm

#endif  // SCM_PARSER_H
