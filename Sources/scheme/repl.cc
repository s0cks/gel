#include "scheme/repl.h"

#include <iostream>

#include "scheme/parser.h"
#include "scheme/runtime.h"

namespace scm {
Repl::Repl(std::istream& is, std::ostream& os, LocalScope* scope) :
  in_(is),
  out_(os),
  scope_(scope) {
  ASSERT(in().good());
  ASSERT(out().good());
  ASSERT(scope_);
  expression_.reserve(Parser::kDefaultChunkSize);
}

auto Repl::Prompt() -> bool {
  out() << ">>> ";
  std::getline(in(), expression_);
  return in().good();
}

static inline auto IsExitCommand(const std::string& cmd) -> bool {
  return cmd == "exit" || cmd == "quit" || cmd == "q";
}

static inline auto IsHelpCommand(const std::string& cmd) -> bool {
  return cmd == "help" || cmd == "h";
}

auto Repl::RunRepl() -> int {
  const auto runtime = GetRuntime();
  ASSERT(runtime);
  runtime->SetRunning();
  while (runtime->IsRunning() && Prompt()) {
    if (IsExitCommand(expression_)) {
      break;
    } else if (IsHelpCommand(expression_)) {
      // TODO: print help
      Respond("No help available.");
      continue;
    }

    if (expression_.empty()) {
      Respond("Nothing to eval.");
      continue;
    }

    const auto result = Runtime::Eval(expression_);
    ASSERT(result);
    Respond(result);
  }
  return EXIT_SUCCESS;
}
}  // namespace scm