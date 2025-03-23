#ifndef GEL_REPL_H
#define GEL_REPL_H

#include <iostream>
#include <string>

#include "gel/error.h"
#include "gel/local_scope.h"
#include "gel/object.h"
#include "gel/parser.h"

namespace gel {
#if defined(OS_IS_OSX) || defined(OS_IS_LINUX)

#include <ncurses.h>

#else
#error "Unsupported Operating System"
#endif

class Repl {
  static constexpr const auto kDefaultReplHistoryLength = 50;
  static constexpr const auto kDefaultReplBufferLength = Parser::kDefaultChunkSize;
  DEFINE_NON_COPYABLE_TYPE(Repl);

 private:
#if defined(OS_IS_OSX) || defined(OS_IS_LINUX)

  WINDOW* window_ = nullptr;

#else
#error "Unsupported Operating System"
#endif
  LocalScope* scope_;
  std::string expression_{};
  bool running_ = false;
  std::vector<std::string> history_{};

  auto Prompt(const std::string& prompt) -> std::string;

  void SetRunning(const bool rhs = true) {
    running_ = rhs;
  }

  void PrintCR();
  void ClearScreen();
  void PrintBanner();
  void PrintHelp();
  void RefreshLine(const std::string& line, const std::string& prompt);

 public:
  explicit Repl(LocalScope* scope);
  ~Repl() = default;

#if defined(OS_IS_OSX) || defined(OS_IS_LINUX)

  auto GetWindowHandle() const -> WINDOW* {
    return window_;
  }

#else
#error "Unsupported Operating System"
#endif

  auto GetScope() const -> LocalScope* {
    return scope_;
  }

  auto IsRunning() const -> bool {
    return running_;
  }

  auto Run() -> int;
  void Print(std::string value);
  void Print(Object* value);
  void Terminate();

  inline void Print(const std::stringstream& ss) {
    return Print(std::move(ss.str()));
  }

 public:
  static void Init(LocalScope* scope = LocalScope::New());
};

auto IsReplInitializedForCurrentThread() -> bool;
auto GetReplForCurrentThread() -> Repl*;
}  // namespace gel

#endif  // GEL_REPL_H
