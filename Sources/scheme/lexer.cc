#include "scheme/lexer.h"

#include <cstdio>

namespace scm {
auto TokenStream::Peek() -> const Token& {
  if (!peek_.IsInvalid())
    return peek_;
  ASSERT(peek_.IsInvalid());
  peek_ = Next();
  return peek_;
}

static inline auto IsValidIdentifierChar(const char c, const bool initial = false) -> bool {
  if (isalpha(c))
    return true;
  else if (isdigit(c) && !initial)
    return true;
  switch (c) {
    case '!':
    case '$':
    case '%':
    case '&':
    case '*':
    case '/':
    case ':':
    case '<':
    case '=':
    case '>':
    case '?':
    case '~':
    case '_':
    case '^':
    case '+':
    case '-':
      return true;
    case '.':
      return !initial;
  }
  return false;
}

static inline auto IsDoubleQuote(const char c) -> bool {
  return c == '\"';
}

static inline auto IsValidStringCharacter(const char c) -> bool {
  return c != EOF && !IsDoubleQuote(c);
}

auto TokenStream::Next() -> const Token& {
  if (!peek_.IsInvalid()) {
    next_ = peek_;
    peek_ = Token{};
    return next_;
  }

  const auto next = PeekChar();
  switch (next) {
    case '(':
      Advance();
      return NextToken(Token::kLParen);
    case ')':
      Advance();
      return NextToken(Token::kRParen);
    case '+':
      Advance();
      return NextToken(Token::kPlus);
    case '-':
      Advance();
      return NextToken(Token::kMinus);
    case '*':
      Advance();
      return NextToken(Token::kMultiply);
    case '/':
      Advance();
      return NextToken(Token::kDivide);
    case '%':
      Advance();
      return NextToken(Token::kModulus);
    case '=':
      Advance();
      return NextToken(Token::kEquals);
    case '#': {
      switch (tolower(PeekChar(1))) {
        case 'f':
          Advance(2);
          return NextToken(Token::kLiteralFalse);
        case 't':
          Advance(2);
          return NextToken(Token::kLiteralTrue);
      }
      return NextToken(Token::kHash, '#');
    }
    case '\n':
    case '\t':
    case '\r':
    case ' ':
      Advance();
      return Next();
    case '\'':
      Advance();
      return NextToken(Token::kQuote, '\'');
    case ';':
      AdvanceUntil('\n');
      return Next();
    case EOF:
      return NextToken(Token::kEndOfStream);
  }

  if (IsDoubleQuote(next)) {
    Advance();
    uint64_t token_len = 0;
    while (IsValidStringCharacter(PeekChar())) {
      buffer_[token_len++] = NextChar();
    }
    ASSERT(IsDoubleQuote(PeekChar()));
    Advance();
    return NextToken(Token::kLiteralString, std::string((const char*)&buffer_[0], token_len));
  } else if (isdigit(next)) {
    bool whole = true;
    uint64_t token_len = 0;
    while (isdigit(PeekChar()) || (next == '.' && whole)) {
      buffer_[token_len++] = NextChar();
      if (next == '.')
        whole = false;
    }
    const auto value = std::string((const char*)&buffer_[0], token_len);
    return whole ? NextToken(Token::kLiteralLong, value) : NextToken(Token::kLiteralDouble, value);
  } else if (IsValidIdentifierChar(next, true)) {
    uint64_t token_len = 0;
    while (IsValidIdentifierChar(PeekChar(), token_len == 0)) {
      buffer_[token_len++] = NextChar();
    }

    const std::string ident((const char*)&buffer_[0], token_len);
    if (ident == "define")
      return NextToken(Token::kLocalDef);
    else if (ident == "defmodule")
      return NextToken(Token::kModuleDef);
    else if (ident == "import")
      return NextToken(Token::kImportDef);
    else if (ident == "begin")
      return NextToken(Token::kBeginExpr);
    else if (ident == "add")
      return NextToken(Token::kPlus);
    else if (ident == "subtract")
      return NextToken(Token::kMinus);
    else if (ident == "multiply")
      return NextToken(Token::kMultiply);
    else if (ident == "divide")
      return NextToken(Token::kDivide);
    else if (ident == "lambda")
      return NextToken(Token::kLambdaExpr);
    else if (ident == "quote")
      return NextToken(Token::kQuote);
    else if (ident == "eq?")
      return NextToken(Token::kEquals);
    else if (ident == "cond")
      return NextToken(Token::kCond);
    return NextToken(Token::kIdentifier, ident);
  }

  return NextToken(Token::kInvalid, GetRemaining());
}
}  // namespace scm