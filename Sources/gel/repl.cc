#include "gel/repl.h"

#include <iostream>

#include "gel/common.h"
#include "gel/type/error.h"
#include "gel/type/object.h"
#include "gel/type/module.h"
#include "gel/frontend/parser.h"
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
  
void Repl::ClearOut() {
#if defined(OS_IS_OSX) || defined(OS_IS_LINUX)
  system("clear");
#elif defined(OS_IS_WINDOWS)
  system("cls");
#else
#error "Unsupported Operating System"
#endif
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
  SetRunning();
  while (IsRunning() && Prompt()) {
    if (IsExitCommand(expression_)) {
      SetRunning(false);
      continue;
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

    const auto [result, duration] = TimedExecution<Object*>([this]() {
      try {
        return Runtime::Eval(expression_);
      } catch (const gel::Exception& exc) {
        return (Object*)Error::New(exc.GetMessage());
      }
    });
    if (!result->IsNil())
      Respond(result);
    if (VLOG_IS_ON(10))
      out() << "finished in " << units::time::nanosecond_t(static_cast<double>(duration.count())) << std::endl;
  }
  return EXIT_SUCCESS;
}

auto Repl::Run(std::istream& is, std::ostream& os) -> int {
  return RunWithScope(is, os, LocalScope::New());
}
  
void Repl::Respond(Error* rhs) {
  out() << std::endl;
  out() << "Error: " << rhs->AsError()->GetMessage()->Get() << std::endl;
}

void Repl::Respond(Object* rhs) {
  ASSERT(rhs);
  if (rhs->IsError())
    return Respond(rhs->AsError());
  out() << std::endl;
  if (VLOG_IS_ON(10))
    out() << "Result: ";
  PrintValue(out(), rhs) << std::endl;
}
}  // namespace gel
