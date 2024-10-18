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
    kIdentifier,
    kPlus,
    kMinus,
    kMultiply,
    kDivide,
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

class TokenStream {
  DEFINE_NON_COPYABLE_TYPE(TokenStream);

 public:
  static constexpr const auto kChunkSize = 4096;
  using Chunk = std::array<uint8_t, kChunkSize>;

 protected:
  Position pos_{};
  Chunk chunk_{};
  uint64_t wpos_ = 0;
  uint64_t rpos_ = 0;
  std::vector<char> buffer_;
  Token next_;
  Token peek_;

  TokenStream() = default;
  TokenStream(const uint8_t* data, const uint64_t length) :
    buffer_(),
    wpos_(length) {
    ASSERT(data);
    ASSERT(length >= 1 && length <= kChunkSize);
    memcpy(&chunk_[0], &data[0], length);  // NOLINT
    buffer_.reserve(1024);
  }

  inline auto GetRemaining() const -> std::string {
    return {(const char*)&chunk_.at(rpos_), wpos_ - rpos_};  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  inline auto NextToken(const Token::Kind kind) -> const Token& {
    return next_ = Token{
               .kind = kind,
               .pos = pos_,
           };
  }

  inline auto NextToken(const Token::Kind kind, const std::string& text) -> const Token& {
    return next_ = Token{
               .kind = kind,
               .pos = pos_,
               .text = text,
           };
  }

  inline auto NextToken(const Token::Kind kind, const char c) -> const Token& {
    return NextToken(kind, std::string(1, c));
  }

  void SetChunk(const Chunk& chunk) {
    chunk_ = chunk;
  }

  inline void Advance(uint64_t n = 1) {
    while (n-- > 0) NextChar();
  }

  inline auto AdvanceUntil(const char expected) -> uint64_t {
    uint64_t advanced = 0;
    while (PeekChar() != expected) {
      NextChar();
      advanced++;
    }
    return advanced;
  }

  inline auto PeekChar(const uint64_t offset = 0) const -> char {
    const auto idx = rpos_ + offset;
    if (idx >= wpos_)
      return EOF;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return static_cast<char>(chunk_.at(idx));
  }

  inline auto NextChar() -> char {
    if ((rpos_ + 1) > wpos_)
      return EOF;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const auto next = chunk_.at(rpos_++);
    switch (next) {
      case '\n':
        pos_.row += 1;
        pos_.column = 1;
        break;
      default:
        pos_.column += 1;
    }
    return static_cast<char>(next);
  }

 public:
  virtual ~TokenStream() = default;
  virtual auto Peek() -> const Token&;
  virtual auto Next() -> const Token&;
};

class ByteTokenStream : public TokenStream {
  DEFINE_NON_COPYABLE_TYPE(ByteTokenStream);

 public:
  explicit ByteTokenStream(const Chunk& chunk) :
    TokenStream() {
    SetChunk(chunk);
  }
  ByteTokenStream(const uint8_t* data, const uint64_t length) :
    TokenStream(data, length) {}
  explicit ByteTokenStream(const std::string& chunk) :
    ByteTokenStream((const uint8_t*)chunk.data(), chunk.length()) {}  // NOLINT
  ~ByteTokenStream() override = default;
};
}  // namespace scm

#endif  // SCM_TOKEN_H
