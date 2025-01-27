#ifndef GEL_REPL_H
#define GEL_REPL_H

#include <iostream>

#include "gel/error.h"
#include "gel/local_scope.h"

namespace gel {
class Repl {
  DEFINE_NON_COPYABLE_TYPE(Repl);

 private:
  std::istream& in_;
  std::ostream& out_;
  LocalScope* scope_;
  std::string expression_{};
  bool running_ = false;

  auto Prompt() -> bool;

  void SetRunning(const bool rhs = true) {
    running_ = rhs;
  }

  inline void Respond(Error* rhs) {
    out() << std::endl;
    out() << "Error: " << rhs->AsError()->GetMessage()->Get() << std::endl;
  }

  inline void Respond(const Exception& rhs) {
    out() << std::endl;
    out() << "Error: " << rhs.what() << std::endl;
  }

  inline void Respond(Object* rhs) {
    ASSERT(rhs);
    if (rhs->IsError())
      return Respond(rhs->AsError());
    out() << std::endl;
    if (VLOG_IS_ON(10))
      out() << "Result: ";
    PrintValue(out(), rhs) << std::endl;
  }

  inline void Respond(const std::string& rhs) {
    ASSERT(!rhs.empty());
    out() << std::endl << rhs << std::endl;
  }

  inline void ClearOut() {
#if defined(OS_IS_OSX) || defined(OS_IS_LINUX)
    system("clear");
#elif defined(OS_IS_WINDOWS)
    system("cls");
#else
#error "Unsupported Operating System"
#endif
  }

  inline auto in() const -> std::istream& {
    return in_;
  }

  inline auto out() const -> std::ostream& {
    return out_;
  }

 public:
  explicit Repl(std::istream& in, std::ostream& out, LocalScope* scope);
  ~Repl() = default;

  auto GetScope() const -> LocalScope* {
    return scope_;
  }

  auto IsRunning() const -> bool {
    return running_;
  }

  auto RunRepl() -> int;

 public:
  // TODO: clean this function up
  static inline auto Run(std::istream& is = std::cin, std::ostream& os = std::cout, LocalScope* scope = LocalScope::New())
      -> int {
    ASSERT(scope);
    Repl repl(is, os, scope);
    return repl.RunRepl();
  }
};
}  // namespace gel

#endif  // GEL_REPL_H
