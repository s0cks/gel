#include "call_stack.h"

namespace gel {
auto CallStack::Pop() -> StackFrame* {
  LOG_IF(FATAL, IsEmpty()) << "stack underflow";
  const auto top = data_.top();
  data_.pop();
  return top;
}

auto CallStack::Push(Object* target, LocalScope* locals) -> StackFrame* {
  ASSERT(target);
  ASSERT(locals);
  data_.push(new StackFrame(GetNextId(), target, locals));
  return data_.top();
}
}  // namespace gel
