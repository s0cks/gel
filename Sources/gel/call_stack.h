#ifndef GEL_CALL_STACK_H
#define GEL_CALL_STACK_H

#include <stack>

#include "common.h"
#include "local_scope.h"
#include "stack_frame.h"

namespace gel {
class CallStack {
  using Stack = std::stack<StackFrame*>;
  DEFINE_DEFAULT_COPYABLE_TYPE(CallStack);

 public:
  class Iterator {
    DEFINE_NON_COPYABLE_TYPE(Iterator);

   private:
    Stack frames_{};

   public:
    explicit Iterator(const CallStack& stack) :
      frames_(stack) {}
    ~Iterator() = default;

    auto HasNext() const -> bool {
      return !frames_.empty();
    }

    auto Next() -> StackFrame* {
      const auto next = frames_.top();
      frames_.pop();
      return next;
    }
  };

 private:
  Stack data_{};

 protected:
  auto PopStackFrame() -> StackFrame*;
  auto Push(Object* target, LocalScope* locals) -> StackFrame*;

  inline auto GetNextId() const -> uword {
    return !IsEmpty() ? GetTop()->GetId() + 1 : 1;
  }

 public:
  CallStack() = default;
  ~CallStack() = default;

  inline auto GetTop() const -> StackFrame* {
    return data_.top();
  }

  inline auto IsEmpty() const -> bool {
    return data_.empty();
  }

  auto Pop() -> StackFrame*;

  template <StackFrameTarget Target>
  auto PushStackFrame(Target* target, LocalScope* locals) -> StackFrame* {
    ASSERT(target);
    ASSERT(locals);
    return Push(target, locals);
  }

  inline auto operator->() const -> StackFrame* {
    return GetTop();
  }

  inline operator Stack() const {
    return data_;
  }
};
}  // namespace gel

#endif  // GEL_CALL_STACK_H
