#ifndef SCM_TOKEN_H
#define SCM_TOKEN_H

#include <cstdint>
#include <ostream>

#include "scheme/common.h"

namespace scm {
struct Position {
  uint64_t row;
  uint64_t column;

  friend auto operator<<(std::ostream& stream, const Position& rhs) -> std::ostream& {
    return stream << "(" << rhs.row << ", " << rhs.column << ")";
  }

  auto operator==(const Position& rhs) const -> bool {
    return row == rhs.row && column == rhs.column;
  }

  auto operator!=(const Position& rhs) const -> bool {
    return row != rhs.row || column != rhs.column;
  }
};

#define FOR_EACH_TOKEN(V) \
  V(Comment)              \
  V(LocalDef)             \
  V(ModuleDef)            \
  V(ImportDef)            \
  V(MacroDef)             \
  V(LambdaExpr)           \
  V(BeginExpr)            \
  V(SetExpr)              \
  V(ConsExpr)             \
  V(CarExpr)              \
  V(CdrExpr)              \
  V(ThrowExpr)            \
  V(Plus)                 \
  V(Minus)                \
  V(Multiply)             \
  V(Divide)               \
  V(Modulus)              \
  V(Hash)                 \
  V(Quote)                \
  V(Equals)               \
  V(DoubleQuote)          \
  V(Cond)                 \
  V(Not)                  \
  V(And)                  \
  V(Or)                   \
  V(LParen)               \
  V(RParen)               \
  V(Identifier)           \
  V(LiteralNumber)        \
  V(LiteralDouble)        \
  V(LiteralLong)          \
  V(LiteralTrue)          \
  V(LiteralFalse)         \
  V(LiteralString)

struct Token {
 public:
  enum Kind : int16_t {
    kEndOfStream = -1,
    kInvalid = 0,
#define DEFINE_TOKEN(Name) k##Name,
    FOR_EACH_TOKEN(DEFINE_TOKEN)
#undef DEFINE_TOKEN
  };

  friend auto operator<<(std::ostream& stream, const Kind& rhs) -> std::ostream& {
    switch (rhs) {
      case kEndOfStream:
        return stream << "EndOfStream";
#define DEFINE_TO_STRING(Name) \
  case Kind::k##Name:          \
    return stream << #Name;
        FOR_EACH_TOKEN(DEFINE_TO_STRING)
#undef DEFINE_TO_STRING
      default:
        return stream << "Unknown Token::Kind: " << static_cast<uint16_t>(rhs);
    }
  }

 public:
  Kind kind = kInvalid;
  Position pos{};
  std::string text{};

  auto IsInvalid() const -> bool {
    return kind == kInvalid;
  }

  auto IsEndOfStream() const -> bool {
    return kind == kEndOfStream;
  }

  auto IsLiteral() const -> bool {
    switch (kind) {
      case kLiteralTrue:
      case kLiteralFalse:
      case kLiteralString:
      case kLiteralNumber:
      case kLiteralLong:
      case kLiteralDouble:
        return true;
      default:
        return false;
    }
  }

  auto AsDouble() const -> double {
    return atof(text.data());
  }

  auto AsLong() const -> uint64_t {
    return atol(text.data());
  }

  auto AsInt() const -> uint32_t {
    return atoi(text.data());
  }

  friend auto operator<<(std::ostream& stream, const Token& rhs) -> std::ostream& {
    stream << "Token(";
    stream << "kind=" << rhs.kind << ", ";
    stream << "pos=" << rhs.pos << ", ";
    if (!rhs.text.empty())
      stream << "text=" << rhs.text;
    stream << ")";
    return stream;
  }
};
}  // namespace scm

#endif  // SCM_TOKEN_H
