#include "gel/repl.h"

#include <ncurses.h>

#include <algorithm>
#include <exception>
#include <iostream>

#include "gel/common.h"
#include "gel/gel.h"
#include "gel/module.h"
#include "gel/object.h"
#include "gel/runtime.h"
#include "gel/thread_local.h"

namespace gel {
Repl::Repl(LocalScope* scope) :
  scope_(scope) {
  ASSERT(scope_);
  history_.reserve(kDefaultReplHistoryLength);
  expression_.reserve(kDefaultReplBufferLength);

  initscr();
  cbreak();
  noecho();

  window_ = newwin(0, 0, 0, 0);
  ASSERT(window_);
  nodelay(window_, true);
  keypad(window_, true);

  PrintBanner();
  PrintCR();
}

void Repl::RefreshLine(const std::string& line, const std::string& prompt) {
  int x = 0;
  int y = 0;
  getyx(window_, y, x);
  wmove(window_, y, 0);
  wclrtobot(window_);
  wprintw(window_, "%s %s", prompt.c_str(), line.c_str());
  wrefresh(window_);
}

void Repl::PrintCR() {
  wprintw(window_, "\n");
  wrefresh(window_);
}

auto Repl::NextHistoryItem(int ch) -> std::string {
  switch (ch) {
    case KEY_UP:
      IncHistoryIndex();
      break;
    case KEY_DOWN:
      DecHistoryIndex();
      break;
  }
  if (history_index_ == -1)
    return {};
  return history_[history_.size() - 1 - history_index_];
}

auto Repl::Prompt(const std::string& prompt) -> std::string& {
  PrintCR();
  RefreshLine(expression_, prompt);
  int ch = 0;
  bool eoc = false;
  while (!eoc) {
    switch (ch = wgetch(window_)) {
      case KEY_UP:
      case KEY_DOWN: {
        expression_ = NextHistoryItem(ch);
        break;
      }
      case 127: {
        if (!expression_.empty())
          expression_.pop_back();
        break;
      }
      case 10: {
        eoc = true;
        break;
      }
      default: {
        if (ch != -1)
          expression_ += static_cast<char>(ch);
        break;
      }
    }
    RefreshLine(expression_, prompt);
  }
  return expression_;
}

void Repl::ClearScreen() {
  wclear(window_);
  wmove(window_, 0, 0);
  PrintBanner();
}

static inline auto IsExitCommand(const std::string& cmd) -> bool {
  return cmd == "exit" || cmd == "quit" || cmd == "q";
}

static inline auto IsHelpCommand(const std::string& cmd) -> bool {
  return cmd == "help" || cmd == "h";
}

static inline auto IsClearCommand(const std::string& cmd) -> bool {
  return cmd == "clear" || cmd == "cls";
}

void Repl::PrintBanner() {
  const auto version = gel::GetVersion();
  wprintw(window_, "gel v%s repl. Type 'exit' to exit.", version.c_str());
}

void Repl::Print(std::string value) {
  ASSERT(!value.empty());
  wprintw(window_, "%s", value.c_str());
}

void Repl::Print(Object* value) {
  std::stringstream ss{};
  PrintValue(ss, value);
  return Print(std::move(ss.str()));
}

void Repl::Terminate() {
  SetRunning(false);
}

void Repl::PrintHelp() {
  Print("No help available.");  // TODO: print help
}

void Repl::EvalExpr() {
  const auto [result, duration] = TimedExecution<Object*>([this]() {
    try {
      return Runtime::Eval(expression_);
    } catch (const gel::Exception& exc) {
      return (Object*)Error::New(exc.GetMessage());
    }
  });
  if (!gel::IsNull(result)) {
    Print(result);
    if (VLOG_IS_ON(10)) {
      // do nothing
    }
    // out() << "finished in " << units::time::nanosecond_t(static_cast<double>(duration.count())) << std::endl;
  }
}

auto Repl::Run() -> int {
  SetRunning();
  while (IsRunning()) {
    try {
      const auto& command = Prompt(">>>");
      PrintCR();
      if (command.empty()) {
        Print("Nothing to eval");
        goto next;  // NOLINT(cppcoreguidelines-avoid-goto)
      }

      if (IsExitCommand(command)) {
        goto terminate;  // NOLINT(cppcoreguidelines-avoid-goto)
      } else if (IsHelpCommand(command)) {
        PrintHelp();
        goto next;  // NOLINT(cppcoreguidelines-avoid-goto)
      } else if (IsClearCommand(command)) {
        ClearScreen();
        goto next;  // NOLINT(cppcoreguidelines-avoid-goto)
      }

      history_.push_back(command);
      EvalExpr();
    next:
      expression_.clear();
      PrintCR();
    } catch (...) {
      Print(Error::New(std::current_exception()));
    }
  }

terminate:
  endwin();
  return EXIT_SUCCESS;
}

static ThreadLocal<Repl> instance_{};

auto GetReplForCurrentThread() -> Repl* {
  ASSERT(instance_);
  return instance_.Get();
}

auto IsReplInitializedForCurrentThread() -> bool {
  return instance_.Has();
}

void Repl::Init(LocalScope* scope) {
  ASSERT(scope);
  ASSERT(!IsReplInitializedForCurrentThread());
  instance_.Set(new Repl(scope));
  ASSERT(IsReplInitializedForCurrentThread());
}
}  // namespace gel