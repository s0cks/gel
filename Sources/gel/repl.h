#ifndef GEL_REPL_H
#define GEL_REPL_H

#include <exception>
#include <iostream>
#include <string_view>

#include "gel/common.h"

namespace gel {
class Error;
class Object;
class LocalScope;
class Repl {
  DEFINE_NON_COPYABLE_TYPE(Repl);

 private:
  std::istream& in_;
  std::ostream& out_;
  LocalScope* scope_;
  std::string expression_{};
  bool running_ = false;
  
  inline auto in() const -> std::istream& {
    return in_;
  }

  inline auto out() const -> std::ostream& {
    return out_;
  }

  void SetRunning(const bool rhs = true) {
    running_ = rhs;
  }


  void ClearOut();
  void Respond(Error* rhs);
  void Respond(Object* rhs);
  auto Prompt() -> bool;

  inline void Respond(const std::exception& rhs) {
    out() << std::endl;
    out() << "Error: " << rhs.what() << std::endl;
  }

  inline void Respond(std::exception_ptr rhs) {
    try {
      std::rethrow_exception(rhs);
    } catch (const std::exception& exc) {
      return Respond(rhs);
    }
  }

  inline void Respond(const std::string_view& rhs) {
    out() << std::endl << rhs << std::endl;
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
  static inline auto RunWithScope(std::istream& is = std::cin, std::ostream& os = std::cout, LocalScope* scope) -> int {
    ASSERT(scope);
    Repl repl(is, os, scope);
    return repl.RunRepl();
  }

  static auto Run(std::istream& is, std::ostream& os = std::cout) -> int;
};
}  // namespace gel

#endif  // GEL_REPL_H
