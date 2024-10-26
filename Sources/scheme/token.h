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

struct Token {
 public:
  enum Kind : int16_t {
    kEndOfStream = -1,
    kInvalid = 0,
    kComment,
    kLParen,
    kRParen,
    kVariableDef,
    kBeginDef,
    kLambdaDef,
    kIdentifier,
    kPlus,
    kMinus,
    kMultiply,
    kDivide,
    kModulus,
    kHash,
    kQuote,
    kEquals,
    kDoubleQuote,
    kCond,
    kLiteralNumber,
    kLiteralDouble,
    kLiteralLong,
    kLiteralTrue,
    kLiteralFalse,
    kLiteralString,
  };

  friend auto operator<<(std::ostream& stream, const Kind& rhs) -> std::ostream& {
    switch (rhs) {
      case kEndOfStream:
        return stream << "EndOfStream";
      case kLParen:
        return stream << "LParen";
      case kRParen:
        return stream << "RParen";
      case kIdentifier:
        return stream << "Identifier";
      case kLiteralNumber:
        return stream << "LiteralNumber";
      case kLiteralLong:
        return stream << "LiteralLong";
      case kLiteralDouble:
        return stream << "LiteralDouble";
      case kLiteralTrue:
        return stream << "LiteralTrue";
      case kLiteralFalse:
        return stream << "LiteralFalse";
      case kLiteralString:
        return stream << "LiteralString";
      case kVariableDef:
        return stream << "VariableDef";
      case kBeginDef:
        return stream << "BeginDef";
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
