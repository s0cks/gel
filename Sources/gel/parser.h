#ifndef GEL_PARSER_H
#define GEL_PARSER_H

#include <glog/logging.h>

#include <istream>
#include <ostream>
#include <utility>

#include "gel/common.h"
#include "gel/expression.h"
#include "gel/instruction.h"
#include "gel/lambda.h"
#include "gel/local_scope.h"
#include "gel/runtime.h"
#include "gel/script.h"
#include "gel/token.h"

namespace gel {
class Parser {
  DEFINE_NON_COPYABLE_TYPE(Parser);

 public:
  static constexpr const auto kDefaultChunkSize = 4096;
  static constexpr const auto kDefaultBufferSize = 1024;
  using Chunk = std::array<char, kDefaultChunkSize>;

 private:
  std::istream& stream_;
  LocalScope* scope_;
  Chunk chunk_{};
  std::string buffer_{};
  Position pos_{};
  uint64_t wpos_ = 0;
  uint64_t rpos_ = 0;
  uint64_t token_len_ = 0;
  uint64_t depth_ = 0;
  Token next_{};
  Token peek_{};
  std::stack<Object*> owner_stack_{};
  Script* script_ = nullptr;

  inline void SetScript(Script* script) {
    ASSERT(script);
    script_ = script;
  }

 protected:
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

  template <const google::LogSeverity Severity = google::FATAL>
  inline auto Unexpected(const Token& actual) -> bool {
    LOG_AT_LEVEL(Severity) << "unexpected: " << actual;
    return false;
  }

  inline auto PeekEq(const Token::Kind rhs) -> bool {
    const auto& peek = PeekToken();
    return peek.kind == rhs;
  }

  inline auto ExpectNext(const Token::Kind rhs) -> const Token& {
    const auto& next = NextToken();
    if (next.kind != rhs)
      Unexpected(rhs, next);
    return next;
  }

  auto ParseLambda(const Token::Kind kind) -> Lambda*;

  auto ParseLocalVariable(LocalVariable** local, expr::Expression** value) -> bool;
  auto ParseLiteralString() -> String*;
  auto ParseSymbol() -> Symbol*;
  auto ParseLoadSymbol() -> LoadVariableInstr*;
  auto ParseArguments(ArgumentSet& args) -> bool;
  auto ParseExpressionList(expr::ExpressionList& expressions) -> bool;
  auto ParseRxOpList(expr::RxOpList& operators) -> bool;
  auto ParseSymbolList(SymbolList& symbols) -> bool;
  auto ParseIdentifier(std::string& result) -> bool;
  auto ParseClauseList(expr::ClauseList& clauses) -> bool;
  auto ParseLiteralValue() -> Datum*;

  // Expressions
  auto ParseSetExpr() -> expr::SetExpr*;
  auto ParseCallProcExpr() -> expr::CallProcExpr*;
  auto ParseLiteralExpr() -> expr::LiteralExpr*;
  auto ParseBeginExpr() -> expr::BeginExpr*;
  auto ParseUnaryExpr() -> expr::UnaryExpr*;
  auto ParseBinaryExpr() -> expr::BinaryOpExpr*;
  auto ParseLambdaExpr() -> expr::LambdaExpr*;
  auto ParseThrowExpr() -> expr::ThrowExpr*;
  auto ParseEvalExpr() -> expr::EvalExpr*;
  auto ParseQuotedExpr() -> expr::Expression*;
  auto ParseWhenExpr() -> expr::WhenExpr*;
  auto ParseCaseExpr() -> expr::CaseExpr*;
  auto ParseWhileExpr() -> expr::WhileExpr*;
  auto ParseCondExpr() -> expr::CondExpr*;
  auto ParseLetExpr() -> expr::LetExpr*;
  auto ParseRxOpExpr() -> expr::RxOpExpr*;
  auto ParseLetRxExpr() -> expr::LetRxExpr*;
  auto ParseListExpr() -> expr::Expression*;
  auto ParseInstanceOfExpr() -> expr::InstanceOfExpr*;
  auto ParseCastExpr() -> expr::CastExpr*;
  auto ParseNewExpr() -> expr::NewExpr*;

  // Definitions
  auto ParseDefinition() -> expr::Definition*;
  auto ParseLocalDef() -> expr::LocalDef*;
  auto ParseDefunExpr() -> expr::LocalDef*;
  auto ParseImportDef() -> expr::ImportDef*;
  auto ParseMacroDef() -> expr::MacroDef*;

  inline auto PeekChar(const uint64_t offset = 0) const -> char {
    const auto idx = (rpos_ + offset);
    if (idx >= wpos_)
      return EOF;
    return static_cast<char>(chunk_[idx]);  // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
  }

  inline auto IsWhitespaceChar(const char c) -> bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == EOF;
  }

  inline void SkipWhitespace() {
    while (IsWhitespaceChar(PeekChar())) NextChar();
  }

  inline auto NextChar() -> char {
    if ((rpos_ + 1) > wpos_) {
      if (!ReadNextChunk())
        return EOF;
      return NextChar();
    }
    const auto next = chunk_[rpos_++];  // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    switch (next) {
      case '\n':
        pos_.row += 1;
        pos_.column = 1;
        break;
      case '(':
        IncrementDepth();
        pos_.column += 1;
        break;
      case ')':
        DecrementDepth();
        pos_.column += 1;
        break;
      default:
        pos_.column += 1;
    }
    return static_cast<char>(next);
  }

  inline void SetDepth(uint64_t depth) {
    ASSERT(depth >= 0);
    depth_ = depth;
  }

  inline auto GetDepth() const -> uint64_t {
    return depth_;
  }

  inline void IncrementDepth() {
    ASSERT((depth_ + 1) >= 0);
    depth_ += 1;
  }

  inline void DecrementDepth() {
    ASSERT((depth_ - 1) >= 0);
    depth_ -= 1;
  }

  auto PeekToken() -> const Token&;
  auto NextToken() -> const Token&;

  inline auto GetBufferedText() const -> std::string {
    return {(const char*)&buffer_[0], token_len_};
  }

  inline auto GetRemaining() const -> std::string {
    const auto remaining_len = std::max((uint64_t)0, wpos_ - rpos_);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, cppcoreguidelines-pro-bounds-constant-array-index)
    return {(const char*)&chunk_[rpos_], remaining_len};
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

  inline void Advance(uint64_t n = 1) {
    while (n-- > 0) NextChar();
  }

  inline auto AdvanceUntil(const char expected) -> uint64_t {
    uint64_t advanced = 0;
    while (PeekChar() != expected && PeekChar() != '\0' && PeekChar() != EOF) {
      NextChar();
      advanced++;
    }
    return advanced;
  }

  inline auto GetStreamSize() const -> uint64_t {
    const auto pos = stream_.tellg();
    stream_.seekg(0, std::ios::end);
    const auto length = stream_.tellg();
    stream_.seekg(pos, std::ios::beg);
    return length;
  }

  inline auto ReadNextChunk() -> bool {
    ASSERT(stream_.good());
    stream_.read(chunk_.data(), kDefaultChunkSize);
    const auto num_read = stream_.gcount();
    return (wpos_ = num_read) >= 1;
  }

  void PopScope();
  auto PushScope() -> LocalScope*;

 public:
  explicit Parser(std::istream& stream, LocalScope* scope) :
    stream_(stream),
    scope_(scope) {
    ASSERT(stream.good());
    ASSERT(scope_);
    buffer_.reserve(kDefaultBufferSize);
    LOG_IF(ERROR, !ReadNextChunk()) << "failed to read chunk from stream.";
  }
  ~Parser() = default;

  auto ParseExpression() -> Expression*;
  auto ParseScript() -> Script*;

 public:
  static inline auto ParseExpr(std::istream& stream, LocalScope* scope = LocalScope::New()) -> expr::Expression* {
    ASSERT(stream.good());
    ASSERT(scope);
    Parser parser(stream, scope);
    return parser.ParseExpression();
  }

  static inline auto ParseExpr(const std::string& expr, LocalScope* scope = LocalScope::New()) -> expr::Expression* {
    ASSERT(!expr.empty());
    ASSERT(scope);
    std::istringstream ss(expr);
    return ParseExpr(ss, scope);
  }

  static inline auto ParseScript(std::istream& stream, LocalScope* scope = LocalScope::New(GetRuntime()->GetGlobalScope()))
      -> Script* {
    ASSERT(stream.good());
    ASSERT(scope);
    Parser parser(stream, scope);
    return parser.ParseScript();
  }
};
}  // namespace gel

#endif  // GEL_PARSER_H
