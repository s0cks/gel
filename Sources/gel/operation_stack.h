#ifndef GEL_OPERATION_STACK_H
#define GEL_OPERATION_STACK_H

#include <stack>

#include "gel/common.h"
#include "gel/object.h"

namespace gel {

class OperationStack {
  friend class Runtime;
  friend class StackFrame;
  friend class Interpreter;
  DEFINE_DEFAULT_COPYABLE_TYPE(OperationStack);

 public:
  using Value = Object*;
  using ValueStack = std::stack<Value>;
  using OptionalValue = std::optional<Value>;

 private:
  ValueStack stack_{};

 protected:
  OperationStack() = default;

 public:
  virtual ~OperationStack() = default;

  auto data() const -> const ValueStack& {
    return stack_;
  }

  auto GetTop() const -> OptionalValue {
    if (stack_.empty())
      return std::nullopt;
    return {stack_.top()};
  }

  auto top() const -> const Value& {
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

  auto Pop() -> OptionalValue {
    if (stack_.empty())
      return std::nullopt;
    const auto next = stack_.top();
    ASSERT(next);
    stack_.pop();
    return {next};
  }

  inline auto PopOr(Value value) -> Value {
    ASSERT(value);
    const auto result = Pop();
    return result ? (*result) : value;
  }

  void Push(Value value) {
    ASSERT(value);
    stack_.push(value);
  }

  inline void PopN(std::vector<Value>& result, const uword num, const bool reverse = false) {
    for (auto idx = 0; idx < num; idx++) {
      result.push_back(PopOr(Null()));
    }
    if (reverse)
      std::ranges::reverse(std::begin(result), std::end(result));
  }
};
}  // namespace gel

#endif  // GEL_OPERATION_STACK_H
