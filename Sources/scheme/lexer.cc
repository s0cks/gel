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

static inline auto IsValidNumberChar(const char c, const bool whole = true) -> bool {
  return isdigit(c) || (c == '.' && whole);
}

static inline auto IsValidQuotedChar(const char c) -> bool {
  switch (c) {
    case EOF:
    case ')':
      return false;
    default:
      return true;
  }
}

auto TokenStream::NextQuote() -> const Token& {
  peek_ = Token{};
  if (PeekChar() != '(') {
    const auto next = NextChar();
    LOG(ERROR) << "unexpected token: " << next;
    return NextToken(Token::kInvalid, next);
  }
  NextChar();

  while (IsValidQuotedChar(PeekChar())) {
    buffer_[token_len_++] = NextChar();
  }

  if (PeekChar() != ')') {
    const auto next = NextChar();
    LOG(ERROR) << "unexpected token: " << next;
    return NextToken(Token::kInvalid, next);
  }
  NextChar();
  return NextToken(Token::kQuotedExpr, GetBufferedText());
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
      depth_ += 1;
      Advance();
      return NextToken(Token::kLParen);
    case ')':
      depth_ -= 1;
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
    case '&':
      Advance();
      return NextToken(Token::kAnd);
    case '|':
      Advance();
      return NextToken(Token::kOr);
    case '!':
      Advance();
      return NextToken(Token::kNot);
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
      return NextToken(Token::kQuote);
    case ';':
      AdvanceUntil('\n');
      return Next();
    case '<': {
      Advance();
      if (PeekChar() == '=') {
        Advance();
        return NextToken(Token::kLessThanEqual);
      }
      return NextToken(Token::kLessThan);
    }
    case '>': {
      Advance();
      if (PeekChar() == '=') {
        Advance();
        return NextToken(Token::kGreaterThanEqual);
      }
      return NextToken(Token::kGreaterThan);
    }
    case EOF:
      return NextToken(Token::kEndOfStream);
  }

  if (IsDoubleQuote(next)) {
    Advance();
    token_len_ = 0;
    while (IsValidStringCharacter(PeekChar())) {
      buffer_[token_len_++] = NextChar();
    }
    ASSERT(IsDoubleQuote(PeekChar()));
    Advance();
    return NextToken(Token::kLiteralString, GetBufferedText());
  } else if (isdigit(next)) {
    token_len_ = 0;
    bool whole = true;
    while (IsValidNumberChar(PeekChar())) {
      const auto next = NextChar();
      buffer_[token_len_++] = next;
      if (next == '.')
        whole = false;
    }
    return whole ? NextToken(Token::kLiteralLong, GetBufferedText()) : NextToken(Token::kLiteralDouble, GetBufferedText());
  } else if (IsValidIdentifierChar(next, true)) {
    token_len_ = 0;
    while (IsValidIdentifierChar(PeekChar(), token_len_ == 0)) {
      buffer_[token_len_++] = NextChar();
    }
    const auto ident = GetBufferedText();
    if (ident == "define")
      return NextToken(Token::kLocalDef);
    else if (ident == "defmodule")
      return NextToken(Token::kModuleDef);
    else if (ident == "defmacro")
      return NextToken(Token::kMacroDef);
    else if (ident == "import")
      return NextToken(Token::kImportDef);
    else if (ident == "cons")
      return NextToken(Token::kConsExpr);
    else if (ident == "car")
      return NextToken(Token::kCarExpr);
    else if (ident == "cdr")
      return NextToken(Token::kCdrExpr);
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
    else if (ident == "not")
      return NextToken(Token::kNot);
    else if (ident == "and")
      return NextToken(Token::kAnd);
    else if (ident == "or")
      return NextToken(Token::kOr);
    else if (ident == "throw")
      return NextToken(Token::kThrowExpr);
    else if (ident == "eq?")
      return NextToken(Token::kEquals);
    else if (ident == "set!")
      return NextToken(Token::kSetExpr);
    else if (ident == "cond")
      return NextToken(Token::kCond);
    return NextToken(Token::kIdentifier, ident);
  }

  return NextToken(Token::kInvalid, GetRemaining());
}
}  // namespace scm