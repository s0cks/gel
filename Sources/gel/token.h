#ifndef GEL_TOKEN_H
#define GEL_TOKEN_H

#include <cstdint>
#include <ostream>

#include "gel/common.h"
#include "gel/expression.h"

namespace gel {
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

#define FOR_EACH_TOKEN(V)     \
  FOR_EACH_EXPRESSION_NODE(V) \
  FOR_EACH_BINARY_OP(V)       \
  FOR_EACH_UNARY_OP(V)        \
  V(Fn)                       \
  V(DefNamespace)             \
  V(DefNative)                \
  V(Def)                      \
  V(Defn)                     \
  V(Comment)                  \
  V(Hash)                     \
  V(Quote)                    \
  V(DoubleQuote)              \
  V(Cond)                     \
  V(LParen)                   \
  V(RParen)                   \
  V(Dot)                      \
  V(Range)                    \
  V(Identifier)               \
  V(LBracket)                 \
  V(RBracket)                 \
  V(LiteralNumber)            \
  V(LiteralDouble)            \
  V(LiteralLong)              \
  V(LiteralTrue)              \
  V(LiteralFalse)             \
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

  auto IsIdentifier() const -> bool {
    return kind == Token::kIdentifier;
  }

  auto IsQuote() const -> bool {
    return kind == Token::kQuote;
  }

  auto IsBinaryOp() const -> bool {
    switch (kind) {
#define DEFINE_OP_CHECK(Name) case Kind::k##Name:
      FOR_EACH_BINARY_OP(DEFINE_OP_CHECK)
#undef DEFINE_OP_CHECK
      return true;
      default:
        return false;
    }
  }

  auto ToBinaryOp() const -> std::optional<expr::BinaryOp> {
    ASSERT(IsBinaryOp());
    switch (kind) {
#define DEFINE_TO_BINARY_OP(Name) \
  case Token::k##Name:            \
    return {BinaryOp::k##Name};
      FOR_EACH_BINARY_OP(DEFINE_TO_BINARY_OP)
#undef DEFINE_TO_BINARY_OP
      default:
        return std::nullopt;
    }
  }

  auto IsUnaryOp() const -> bool {
    switch (kind) {
#define DEFINE_OP_CHECK(Name) case Kind::k##Name:
      FOR_EACH_UNARY_OP(DEFINE_OP_CHECK)
#undef DEFINE_OP_CHECK
      return true;
      default:
        return false;
    }
  }

  auto ToUnaryOp() const -> std::optional<expr::UnaryOp> {
    ASSERT(IsUnaryOp());
    switch (kind) {
#define TO_UNARY_OP(Name) \
  case Token::k##Name:    \
    return {expr::UnaryOp::k##Name};
      FOR_EACH_UNARY_OP(TO_UNARY_OP)
#undef TO_UNARY_OP
      default:
        return std::nullopt;
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

  auto operator==(const Token::Kind& rhs) const -> bool {
    return kind == rhs;
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
}  // namespace gel

#endif  // GEL_TOKEN_H
