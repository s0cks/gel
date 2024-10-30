#ifndef SCM_TOKEN_H
#define SCM_TOKEN_H

#include <cstdint>
#include <ostream>

#include "scheme/common.h"
#include "scheme/expression.h"

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
  V(LessThan)             \
  V(GreaterThan)          \
  V(LessThanEqual)        \
  V(GreaterThanEqual)     \
  V(LParen)               \
  V(RParen)               \
  V(QuotedExpr)           \
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

  auto IsBinaryOp() const -> bool {
    switch (kind) {
      case Kind::kPlus:
      case Kind::kMinus:
      case Kind::kMultiply:
      case Kind::kDivide:
      case Kind::kModulus:
      case Kind::kAnd:
      case Kind::kOr:
      case Kind::kEquals:
      case Kind::kLessThan:
      case Kind::kLessThanEqual:
      case Kind::kGreaterThan:
      case Kind::kGreaterThanEqual:
        return true;
      default:
        return false;
    }
  }

  auto ToBinaryOp() const -> std::optional<expr::BinaryOp> {
    ASSERT(IsBinaryOp());
    switch (kind) {
      case Token::kPlus:
        return {BinaryOp::kAdd};
      case Token::kMinus:
        return {BinaryOp::kSubtract};
      case Token::kMultiply:
        return {BinaryOp::kMultiply};
      case Token::kDivide:
        return {BinaryOp::kDivide};
      case Token::kModulus:
        return {BinaryOp::kModulus};
      case Token::kEquals:
        return {BinaryOp::kEquals};
      case Token::kAnd:
        return {BinaryOp::kBinaryAnd};
      case Token::kOr:
        return {BinaryOp::kBinaryOr};
      case Token::kGreaterThan:
        return {BinaryOp::kGreaterThan};
      case Token::kGreaterThanEqual:
        return {BinaryOp::kGreaterThanEqual};
      case Token::kLessThan:
        return {BinaryOp::kLessThan};
      case Token::kLessThanEqual:
        return {BinaryOp::kLessThanEqual};
      default:
        return std::nullopt;
    }
  }

  auto IsUnaryOp() const -> bool {
    switch (kind) {
      case Kind::kNot:
      case Kind::kCarExpr:
      case Kind::kCdrExpr:
        return true;
      default:
        return false;
    }
  }

  auto ToUnaryOp() const -> std::optional<expr::UnaryOp> {
    ASSERT(IsUnaryOp());
    switch (kind) {
      case Token::kNot:
        return {expr::UnaryOp::kNot};
      case Token::kCarExpr:
        return {expr::UnaryOp::kCar};
      case Token::kCdrExpr:
        return {expr::UnaryOp::kCdr};
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
