#ifndef SCM_STATE_H
#define SCM_STATE_H

#include <stack>

#include "scheme/environment.h"

namespace scm {
class State {
  friend class Interpreter;
  DEFINE_NON_COPYABLE_TYPE(State);

 private:
  std::stack<Type*> stack_{};
  Environment* globals_;

  explicit State(Environment* globals);

  inline void SetGlobals(Environment* env) {
    ASSERT(env);
    globals_ = env;
  }

  inline void Push(Type* value) {
    ASSERT(value);
    stack_.push(value);
  }

  inline auto IsStackEmpty() const -> bool {
    return stack_.empty();
  }

  inline auto Pop() -> Type* {
    if (stack_.empty())
      return nullptr;
    const auto& next = stack_.top();
    ASSERT(next);
    stack_.pop();
    return next;
  }

 public:
  ~State();

  auto GetGlobals() const -> Environment* {
    return globals_;
  }

 public:
  static inline auto New(Environment* env = Environment::New()) -> State* {
    return new State(env);
  }
};
}  // namespace scm

#endif  // SCM_STATE_H
