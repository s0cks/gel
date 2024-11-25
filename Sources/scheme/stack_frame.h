#ifndef SCM_STACK_FRAME_H
#define SCM_STACK_FRAME_H

#include <ostream>

#include "scheme/common.h"
#include "scheme/platform.h"
#include "scheme/procedure.h"

namespace scm {
class StackFrame {
  friend class Interpreter;
  DEFINE_DEFAULT_COPYABLE_TYPE(StackFrame);

 private:
  uint64_t id_;
  LocalScope* locals_;
  uword return_address_;

  StackFrame(const uint64_t id, LocalScope* locals, const uword return_address = UNALLOCATED) :
    id_(id),
    locals_(locals),
    return_address_(return_address) {}

  void SetReturnAddress(const uword addr) {
    ASSERT(addr > UNALLOCATED);
  }

 public:
  ~StackFrame() = default;

  auto GetId() const -> uint64_t {
    return id_;
  }

  auto GetLocals() const -> LocalScope* {
    return locals_;
  }

  auto GetReturnAddress() const -> uword {
    return return_address_;
  }

  auto GetReturnAddressPointer() const -> void* {
    return (void*)GetReturnAddress();  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  inline auto HasReturnAddress() const -> bool {
    return GetReturnAddress() != UNALLOCATED;
  }

  friend auto operator<<(std::ostream& stream, const StackFrame& rhs) -> std::ostream& {
    stream << "StackFrame(";
    stream << "id=" << rhs.GetId() << ", ";
    if (rhs.HasReturnAddress())
      stream << "result=" << rhs.GetReturnAddressPointer() << ", ";
    stream << "num_locals=" << rhs.GetLocals()->GetNumberOfLocals();
    stream << ")";
    return stream;
  }

  auto operator==(const StackFrame& rhs) const -> bool {
    return GetId() == rhs.GetId() && GetReturnAddress() == rhs.GetReturnAddress();
  }
};

class StackFrameIterator {
  DEFINE_NON_COPYABLE_TYPE(StackFrameIterator);

 private:
  std::stack<StackFrame> stack_;

 public:
  explicit StackFrameIterator(const std::stack<StackFrame>& stack) :
    stack_(stack) {}
  ~StackFrameIterator() = default;

  auto HasNext() const -> bool {
    return !stack_.empty();
  }

  auto Next() -> StackFrame {
    const auto next = stack_.top();
    stack_.pop();
    return next;
  }
};
}  // namespace scm

#endif  // SCM_STACK_FRAME_H
