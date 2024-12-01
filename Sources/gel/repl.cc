#include "gel/repl.h"

#include <iostream>

#include "gel/parser.h"
#include "gel/runtime.h"

namespace gel {
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

static inline auto IsClearCommand(const std::string& cmd) -> bool {
  return cmd == "clear" || cmd == "cls";
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
    } else if (IsClearCommand(expression_)) {
      ClearOut();
      continue;
    }

    if (expression_.empty()) {
      Respond("Nothing to eval.");
      continue;
    }

    try {
      const auto result = Runtime::Eval(expression_);
      if (result)
        Respond(result);
    } catch (const gel::Exception& exc) {
      Respond(exc);
    }
  }
  return EXIT_SUCCESS;
}
}  // namespace gel