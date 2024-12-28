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
  V(DefMacro)                 \
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
  V(DotDotDot)                \
  V(Range)                    \
  V(Identifier)               \
  V(LBrace)                   \
  V(RBrace)                   \
  V(LBracket)                 \
  V(RBracket)                 \
  V(Question)                 \
  V(Comma)                    \
  V(Dollar)                   \
  V(Dispatch)                 \
  V(LiteralNumber)            \
  V(LiteralDouble)            \
  V(LiteralLong)              \
  V(LiteralTrue)              \
  V(LiteralFalse)             \
  V(LiteralString)

struct Token {
 public:
  // clang-format off
  enum Kind : int16_t {
    kEndOfStream = -1,
    kInvalid = 0,
#define DEFINE_TOKEN(Name) \
    k##Name,
    FOR_EACH_TOKEN(DEFINE_TOKEN)
#undef DEFINE_TOKEN
    kTotalNumberOfTokens,
  };
  // clang-format on

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

  using KindSet = std::bitset<kTotalNumberOfTokens>;

  static inline constexpr auto SetOf(const Token::Kind a, const Token::Kind b) -> KindSet {
    KindSet data;
    data.set(a);
    data.set(b);
    return data;
  }

  static inline constexpr auto SetOf(const std::vector<Token::Kind>& kinds) -> KindSet {
    KindSet data;
    std::ranges::for_each(kinds, [&data](Token::Kind kind) {
      data.set(kind);
    });
    return data;
  }

  static inline constexpr auto AnyBool() -> KindSet {
    return SetOf(Token::kLiteralTrue, Token::kLiteralFalse);
  }

  static inline constexpr auto AnyNumber() -> KindSet {
    return SetOf(Token::kLiteralDouble, Token::kLiteralLong);
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
      case kLBrace:
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

  auto Test(const KindSet& kinds) const -> bool {
    return kinds.test(kind);
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

static inline auto operator<<(std::ostream& stream, const Token::KindSet& rhs) -> std::ostream& {
  for (auto idx = 0; idx < Token::kTotalNumberOfTokens; idx++) {
    if (rhs.test(idx))
      stream << static_cast<Token::Kind>(idx) << " ";
  }
  return stream;
}
}  // namespace gel

#endif  // GEL_TOKEN_H
