#ifndef SCM_INTERPRETER_H
#define SCM_INTERPRETER_H

#include <stack>

#include "scheme/common.h"
#include "scheme/instruction.h"

namespace scm {
class Interpreter {
  DEFINE_NON_COPYABLE_TYPE(Interpreter);

 private:
  std::stack<Datum*> stack_{};

  inline void Push(Datum* value) {
    ASSERT(value);
    stack_.push(value);
  }

  inline auto Pop() -> Datum* {
    if (stack_.empty())
      return nullptr;
    const auto next = stack_.top();
    stack_.pop();
    return next;
  }

  auto LoadVariable(Variable* variable) -> Datum*;

 public:
  Interpreter() = default;
  ~Interpreter() = default;
  auto Execute(EntryInstr* entry) -> Datum*;
};
}  // namespace scm

#endif  // SCM_INTERPRETER_H
