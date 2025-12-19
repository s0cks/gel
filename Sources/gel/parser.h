#ifndef GEL_PARSER_H
#define GEL_PARSER_H

#include <fstream>
#include <glog/logging.h>
#include <istream>
#include <ostream>
#include <termcolor/termcolor.hpp>
#include <type_traits>
#include <utility>
#include <variant>

#include "common.h"
#include "expr/binding_expr.h"
#include "expr/clause_expr.h"
#include "expr/expression.h"
#include "instruction.h"
#include "lambda.h"
#include "local.h"
#include "local_scope.h"
#include "module_loader.h"
#include "namespace.h"
#include "runtime.h"
#include "script.h"
#include "token.h"
#include "type_traits.h"

namespace gel {
class ParseError {
  DEFINE_DEFAULT_COPYABLE_TYPE(ParseError);

 private:
  std::string message_;
  Position start_;
  Position end_;

 public:
  ParseError(const std::string& message, const Position& start, const Position& end) :
    message_(message),
    start_(start),
    end_(end) {}
  ParseError(const std::string& message, const Position& start) :
    ParseError(message, start, start) {}
  ~ParseError() = default;

  auto GetMessage() const -> const std::string& {
    return message_;
  }

  auto GetStartPos() const -> const Position& {
    return start_;
  }

  auto GetEndPos() const -> const Position& {
    return end_;
  }

  friend auto operator<<(std::ostream& stream, const ParseError& rhs) -> std::ostream& {
    const auto distance = (rhs.GetEndPos() - rhs.GetStartPos());
    if (distance == 0)
      return stream << "ParseError at " << rhs.GetStartPos() << ": " << rhs.GetMessage();
    return stream << "ParseError " << rhs.GetStartPos() << "-" << rhs.GetEndPos() << ": " << rhs.GetMessage();
  }
};

class ParseResult {
  DEFINE_DEFAULT_COPYABLE_TYPE(ParseResult);

 private:
  std::variant<bool, ParseError> data_;

 public:
  ParseResult(const bool success) :
    data_(success) {}
  explicit ParseResult(const ParseError& error) :
    data_(error) {}
  ~ParseResult() = default;

  auto data() const -> const std::variant<bool, ParseError>& {
    return data_;
  }

  auto IsError() const -> bool {
    return std::holds_alternative<ParseError>(data());
  }

  auto GetError() const -> const ParseError& {
    return std::get<ParseError>(data());
  }

  auto IsSuccess() const -> bool {
    return std::holds_alternative<bool>(data()) && std::get<bool>(data());
  }

  operator bool() const {
    return IsSuccess();
  }

  friend auto operator<<(std::ostream& stream, const ParseResult& rhs) -> std::ostream& {
    if (rhs.IsSuccess())
      return stream << "Success";
    return stream << rhs.GetError();
  }
};

class ModuleLoader;
class Parser {
  friend class ParseScope;
  friend class TopLevelScope;
  using Severity = google::LogSeverity;
  DEFINE_NON_COPYABLE_TYPE(Parser);

 public:
  static constexpr const auto kDefaultChunkSize = 4096;
  static constexpr const auto kDefaultBufferSize = 1024;
  using Chunk = std::array<char, kDefaultChunkSize>;

  enum State {
    kParsing,
    kParsingArguments,
    kParsingLiteralMap,
  };

 private:
  static inline auto NewParseError(const std::string& message, const Position& start, const Position& stop)
      -> ParseResult {
    ASSERT(!message.empty());
    return ParseResult(ParseError(message, start, stop));
  }

  static inline auto NewParseError(const std::string& message, const Position& start) -> ParseResult {
    ASSERT(!message.empty());
    return NewParseError(message, start, start);
  }

  inline auto ReturnError(const std::string& message, const Position& start) -> ParseResult {
    ASSERT(!message.empty());
    return NewParseError(message, start, pos_);
  }

  inline auto ReturnError(const std::stringstream& ss, const Position& start) -> ParseResult {
    return ReturnError(ss.str(), start);
  }

  inline auto UnexpectedError(const Token& rhs) -> ParseResult {
    std::stringstream ss;
    ss << "Unexpected " << rhs;
    return NewParseError(ss.str(), rhs.pos);
  }

  inline auto GetWindowStartPos() const -> uword {
    auto curr_pos = rpos_;
    while (curr_pos >= 0) {
      const auto current = chunk_[curr_pos];
      if (current == '\n' || current == '\0' || curr_pos == 0)
        break;
      curr_pos -= 1;
    }
    return curr_pos;
  }

  inline auto GetWindowEndPos() const -> uword {
    auto curr_pos = rpos_;
    while (curr_pos <= wpos_) {
      const auto current = chunk_[curr_pos];
      if (current == '\n' || current == '\0' || curr_pos == wpos_)
        break;
      curr_pos += 1;
    }
    return std::min(curr_pos, wpos_);
  }

  inline auto GetWindowBeforePos(const uword pos) const -> std::string {
    const auto start = GetWindowStartPos();
    return {&chunk_[start], (pos - start)};
  }

  inline auto GetWindowBefore() const -> std::string {
    return GetWindowBeforePos(rpos_ - 1);
  }

  inline auto GetWindowAfterPos(const uword pos) const -> std::string {
    const auto end = GetWindowEndPos();
    return {&chunk_[pos], (end - pos)};
  }

  inline auto GetWindowAfter() const -> std::string {
    return GetWindowAfterPos(rpos_);
  }

  inline auto UnexpectedError(const Token& actual, const Token::Kind expected) -> ParseResult {
    std::stringstream ss;
    ss << termcolor::colorize;
    ss << "unexpected: " << actual.kind << ", expected: " << expected << " at: ";
    ss << GetWindowBefore();
    ss << termcolor::underline << actual.text << termcolor::reset;
    ss << GetWindowAfter();
    return NewParseError(ss.str(), actual.pos);
  }

  inline auto UnexpectedError(const Token& actual, const TokenKindBitSet& expected) -> ParseResult {
    std::stringstream ss;
    ss << termcolor::colorize;
    ss << "unexpected: " << actual.kind << ", expected one of: ";
    for (auto idx = 0; idx < Token::kTotalNumberOfTokens; idx++) {
      if (expected.test(idx))
        ss << static_cast<Token::Kind>(idx) << " ";
    }
    ss << "at: ";
    ss << GetWindowBefore();
    ss << termcolor::underline << actual.text << termcolor::reset;
    ss << GetWindowAfter();
    return NewParseError(ss.str(), actual.pos);
  }

 private:
  std::istream& stream_;
  LocalScope* scope_;
  ModuleLoader* loader_;
  std::vector<char> chunk_;
  std::string buffer_{};
  Object* toplevel_ = nullptr;
  Position pos_{.row = 1, .column = 1};
  uint64_t wpos_ = 0;
  uint64_t rpos_ = 0;
  uint64_t token_len_ = 0;
  uint64_t depth_ = 0;
  Token next_{};
  Token peek_{};
  word dispatched_ = -1;
  State state_ = State::kParsing;

  auto GetModuleLoader() const -> ModuleLoader* {
    return loader_;
  }

  auto GetTopLevel() const -> Object* {
    return toplevel_;
  }

  inline auto HasTopLevel() const -> bool {
    return GetTopLevel() != nullptr;
  }

  void PushTopLevel(Script* rhs);
  void PushTopLevel(Module* rhs);
  void PushTopLevel(Class* rhs);
  void PushTopLevel(Namespace* rhs);
  void PushTopLevel(Macro* rhs);
  void PushTopLevel(Lambda* rhs);
  void PopTopLevel();

 protected:
  auto GetPos() const -> const Position& {
    return pos_;
  }

  inline void SetState(const State rhs) {
    state_ = rhs;
  }

  inline auto GetState() const -> State {
    return state_;
  }

  inline void SetParsingArgs() {
    return SetState(kParsingArguments);
  }

  inline void ClearParsingArgs() {  // TODO: refactor
    return SetState(kParsing);
  }

  inline auto IsParsingArgs() const -> bool {
    return GetState() == kParsingArguments;
  }

  inline void SetParsingLiteralMap() {
    return SetState(kParsingLiteralMap);
  }

  inline void ClearParsingLiteralMap() {  // TODO: refactor
    return SetState(kParsing);
  }

  inline auto IsParsingLiteralMap() const -> bool {
    return GetState() == kParsingLiteralMap;
  }

  inline void SetScope(LocalScope* scope) {
    ASSERT(scope);
    scope_ = scope;
  }

  inline auto GetScope() const -> LocalScope* {
    return scope_;
  }

  inline auto IsDispatching() const -> bool {
    return dispatched_ >= 0;
  }

  inline void SetDispatched(const word rhs) {
    dispatched_ = rhs;
  }

  inline void SetDispatching() {
    return SetDispatched(0);
  }

  inline void ClearDispatched() {
    return SetDispatched(-1);
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

  inline auto PeekKind() -> Token::Kind {
    const auto& token = PeekToken();
    return token.kind;
  }

  inline auto PeekEq(const Token::Kind rhs) -> bool {
    return PeekKind() == rhs;
  }

  inline auto ExpectNext(const Token::Kind rhs) -> const Token& {  // TODO: fix this function
    const auto& next = NextToken();
    if (next.kind != rhs)
      Unexpected(rhs, next);
    return next;
  }

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
    while (IsWhitespaceChar(PeekChar()))
      NextChar();
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

  inline auto GetBufferedText() const -> std::string {
    return {(const char*)&buffer_[0], token_len_};
  }

  inline auto GetRemaining() const -> std::string {
    const auto remaining_len = std::max((uint64_t)0, wpos_ - rpos_);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, cppcoreguidelines-pro-bounds-constant-array-index)
    return {(const char*)&chunk_[rpos_], remaining_len};
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

  inline auto NextToken(const Token::Kind kind) -> const Token& {
    return NextToken(kind, Token::GetChar(kind));
  }

  inline void Advance(uint64_t n = 1) {
    while (n-- > 0)
      NextChar();
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

  inline auto ReadNextChunk(const uint64_t num_bytes = kDefaultChunkSize) -> bool {
    ASSERT(stream_.good());
    chunk_.resize(num_bytes);
    stream_.read(chunk_.data(), static_cast<long>(num_bytes));
    const auto num_read = stream_.gcount();
    rpos_ = 0;
    chunk_.resize(num_read);
    return (wpos_ = num_read) >= 1;
  }

  template <const Severity S = google::FATAL>
  void Unexpected(const Token& actual, const Token::Kind expected) {
    LOG_AT_LEVEL(S) << "unexpected " << actual << ", expected: " << expected;
  }

  template <const Severity S = google::FATAL>
  void Unexpected(const Token& actual, const Token::KindSet& expected) {
    LOG_AT_LEVEL(S) << "unexpected " << actual << ", expected: " << expected;
  }

  auto PeekToken() -> const Token&;
  auto NextToken() -> const Token&;
  void PopScope();
  auto PushScope() -> LocalScope*;
  auto IsValidIdentifierChar(const char c, const bool initial = false) const -> bool;

  template <HasDocstring T>
  auto TryParseDocstring(T* owner) -> ParseResult;

  template <WithSymbol T>
  auto TryParseSymbol(T* owner) -> ParseResult
    requires(HasMutableSymbol<T>);

 protected:
  auto ParseLambda(const Token::Kind kind, Lambda** result) -> ParseResult;
  auto ParseMacro(Macro** result) -> ParseResult;
  auto ParseNamespace(Namespace** result) -> ParseResult;

  auto ParseLoadSymbol() -> LoadLocalInstr*;
  auto ParseArguments(Array<Argument*>** args, const bool bind = false) -> ParseResult;
  auto ParseBinding(expr::BindingExpr** result) -> ParseResult;
  auto ParseBindingList(expr::BindingList& bindings, const bool push_scope = true) -> ParseResult;
  auto ParseExpressionList(expr::ExpressionList& expressions, const bool push_scope = true) -> ParseResult;
  auto ParseClauseList(expr::ClauseList& clauses) -> ParseResult;

  auto ParseLiteralBool(Bool** result) -> ParseResult;
  auto ParseLiteralNumber(Number** result) -> ParseResult;
  auto ParseLiteralString(String** result) -> ParseResult;
  auto ParseLiteralSymbol(Symbol** result) -> ParseResult;
  auto ParseLiteralValue(Object** result) -> ParseResult;
  auto ParseLiteralVec(expr::Expression** result) -> ParseResult;
  auto ParseLiteralSet(expr::Expression** result) -> ParseResult;

  auto ParseLiteralLambda(const Token::Kind kind, expr::LiteralExpr** result) -> ParseResult;
  auto ParseLambdaExpr() -> expr::LambdaExpr*;

  auto ParseSeqExpr(expr::SeqExpr** result, const bool allow_empty = true, const Token::Kind end = Token::kRParen)
      -> ParseResult;
  auto ParseDefNamespace(LocalVariable** local) -> ParseResult;
  auto ParseMap(expr::Expression**) -> ParseResult;
  auto ParseSetExpr(expr::Expression**) -> ParseResult;
  auto ParseCallExpr(expr::Expression**) -> ParseResult;
  auto ParseLiteralExpr(expr::Expression**) -> ParseResult;
  auto ParseDoExpr(expr::Expression**) -> ParseResult;
  auto ParseUnaryOpExpr(expr::Expression**) -> ParseResult;
  auto ParseBinaryExpr(expr::Expression**) -> ParseResult;
  auto ParseThrowExpr(expr::Expression**) -> ParseResult;
  auto ParseWhileExpr(expr::Expression**) -> ParseResult;
  auto ParseCondExpr(expr::Expression**) -> ParseResult;
  auto ParseLetExpr(expr::Expression**) -> ParseResult;
  auto ParseForeachExpr(expr::Expression**) -> ParseResult;
  auto ParseForeachBindingExpr(LocalVariable** local, expr::Expression** value) -> ParseResult;
  auto ParseListExpr(expr::Expression** value) -> ParseResult;
  auto ParseInstanceOfExpr(expr::Expression**) -> ParseResult;
  auto ParseCastExpr(expr::Expression**) -> ParseResult;
  auto ParseNewExpr(expr::Expression**) -> ParseResult;
  auto ParseImportExpr(expr::Expression**) -> ParseResult;
  auto ParseDef(expr::Expression**) -> ParseResult;
  auto ParseSetPairField(const Token& token, expr::Expression**) -> ParseResult;
  auto ParseDefNative(LocalVariable** local) -> ParseResult;
  auto ParseDefn(LocalVariable** local) -> ParseResult;
  auto ParseDefType(LocalVariable** local) -> ParseResult;
  auto ParseDefMacro(LocalVariable** local) -> ParseResult;

  auto ParseExpression(Expression** result, const int depth = 0) -> ParseResult;

 public:
  explicit Parser(std::istream& stream, LocalScope* scope, ModuleLoader* loader) :
    stream_(stream),
    scope_(scope),
    loader_(loader) {
    ASSERT(stream.good());
    ASSERT(scope_);
    ASSERT(loader_);
    const auto total_size = GetStreamSize();
    chunk_.reserve(total_size);
    buffer_.reserve(kDefaultBufferSize);
    LOG_IF(ERROR, !ReadNextChunk(total_size)) << "failed to read chunk from stream.";
  }
  ~Parser() = default;

  auto ParseScript(Script** result) -> ParseResult;
  auto ParseModule(const std::string& name, Module** result) -> ParseResult;

 public:
  static inline auto ParseExpr(std::istream& stream, LocalScope* scope = LocalScope::New(GetRuntime()->GetInitScope()))
      -> Lambda* {
    ASSERT(stream.good());
    ASSERT(scope);
    Parser parser(stream, scope, GetThreadModuleLoader());
    expr::Expression* result = nullptr;
    if (!parser.ParseExpression(&result))
      return nullptr;
    ASSERT(result);
    const auto lambda = Lambda::New();
    ASSERT(lambda);
    lambda->SetBody(expr::SeqExpr::New(result));
    lambda->SetScope(scope);
    DLOG(INFO) << "parsed expr scope: ";
    PRINT_SCOPE(INFO, lambda->GetScope());
    return lambda;
  }

  static inline auto ParseExpr(const std::string& expr,
                               LocalScope* scope = LocalScope::New(GetRuntime()->GetInitScope())) -> Lambda* {
    ASSERT(!expr.empty());
    ASSERT(scope);
    std::istringstream ss(expr);
    return ParseExpr(ss, scope);
  }

  static inline auto ParseScript(std::istream& stream,
                                 LocalScope* scope = LocalScope::New(GetRuntime()->GetInitScope())) -> Script* {
    ASSERT(stream.good());
    ASSERT(scope);
    Parser parser(stream, scope, GetThreadModuleLoader());
    Script* script = nullptr;
    const auto result = parser.ParseScript(&script);
    if (!result) {
      LOG(ERROR) << "failed to parse script: " << result;
      return nullptr;
    }
    ASSERT(script);
    return script;
  }

  static inline auto ParseModuleFrom(const std::string& filename,
                                     LocalScope* scope = LocalScope::New(GetRuntime()->GetInitScope()),
                                     ModuleLoader* loader = GetThreadModuleLoader()) -> Module* {
    std::stringstream code;
    {
      std::ifstream file(filename, std::ios::binary | std::ios::in);
      LOG_IF(FATAL, !file) << "failed to load module from: " << filename;
      code << file.rdbuf();
      file.close();
    }
    ASSERT(code.good());
    ASSERT(scope);
    Parser parser(code, scope, loader);
    const auto slashpos = filename.find_last_of('/') + 1;
    const auto dotpos = filename.find_first_of('.', slashpos);
    const auto total_length = (dotpos - slashpos);
    const auto name = filename.substr(slashpos, total_length);

    Module* new_module = nullptr;
    const auto result = parser.ParseModule(name, &new_module);
    if (!result) {
      LOG(ERROR) << "failed to parse Module from " << filename << ": " << result;
      return nullptr;
    }
    ASSERT(new_module);
    return new_module;
  }

 public:
  static void Init();
};
}  // namespace gel

#endif  // GEL_PARSER_H
