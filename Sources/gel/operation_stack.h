#ifndef GEL_OPERATION_STACK_H
#define GEL_OPERATION_STACK_H

#include <stack>

#include "gel/object.h"

namespace gel {

using Stack = std::stack<Object*>;

class OperationStack {
  friend class Runtime;
  friend class StackFrame;
  friend class Interpreter;
  DEFINE_DEFAULT_COPYABLE_TYPE(OperationStack);

 private:
  Stack stack_{};

 protected:
  OperationStack() = default;

  inline void SetStack(const Stack& rhs) {
    ASSERT(!rhs.empty());
    stack_ = rhs;
  }

  inline auto StackTop() const -> std::optional<Stack::value_type> {
    if (stack_.empty())
      return std::nullopt;
    return {stack_.top()};
  }

  inline auto stack() const -> const Stack& {
    return stack_;
  }

 public:
  virtual ~OperationStack() = default;

  auto top() const -> const Stack::value_type& {
    return stack_.top();
  }

  auto IsEmpty() const -> bool {
    return stack_.empty();
  }

  auto GetStackSize() const -> uint64_t {
    return stack_.size();
  }

  auto GetError() const -> Error* {
    ASSERT(HasError());
    return stack_.top()->AsError();
  }

  inline auto HasError() const -> bool {
    if (stack_.empty())
      return false;
    return stack_.top()->IsError();
  }

  auto Pop() -> Stack::value_type {
    if (stack_.empty())
      return nullptr;
    const auto next = stack_.top();
    ASSERT(next);
    stack_.pop();
    return next;
  }

  void Push(Stack::value_type value) {
    ASSERT(value);
    stack_.push(value);
  }

  inline void PopN(std::vector<Object*>& result, const uword num, const bool reverse = false) {
    for (auto idx = 0; idx < num; idx++) {
      result.push_back(Pop());
    }
    if (reverse)
      std::ranges::reverse(std::begin(result), std::end(result));
  }
};
}  // namespace gel

#endif  // GEL_OPERATION_STACK_H
