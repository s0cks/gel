#ifndef GEL_OPERATION_STACK_H
#define GEL_OPERATION_STACK_H

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <optional>
#include <stack>
#include <vector>

#include "common.h"
#include "object.h"
#include "pair.h"
#include "platform.h"
#include "rx.h"
#include "type.h"

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

  class Iterator {
    DEFINE_NON_COPYABLE_TYPE(Iterator);

   private:
    ValueStack stack_;

   public:
    Iterator(const OperationStack& stack) :
      stack_(stack.data()) {}
    ~Iterator() = default;

    auto HasNext() const -> bool {
      return !stack_.empty();
    }

    auto Next() -> Value {
      const auto next = stack_.top();
      stack_.pop();
      return next;
    }
  };

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

  inline void Push(const OptionalValue& rhs, const bool allow_null = false) {
    if (!allow_null)
      ASSERT(rhs);
    Push(allow_null ? rhs.value_or(Nil::Get()) : rhs.value());
  }

  void Dup() {
    ASSERT(!IsEmpty());
    Push(top());
  }

  void Dup2() {
    ASSERT(GetStackSize() >= 2);
    const auto t = Pop();
    ASSERT(t);
    const auto new_top = top();
    ASSERT(new_top);
    Push(t);
    Push(new_top);
    Push(t);
  }

  inline void PopN(std::vector<Value>& result, const uword num, const bool reverse = false) {
    for (uword idx = 0; idx < num; idx++)
      result.push_back(PopOr(Nil::Get()));
    if (reverse)
      std::ranges::reverse(std::begin(result), std::end(result));
  }
};
}  // namespace gel

#endif  // GEL_OPERATION_STACK_H
