#ifndef SCM_STACK_FRAME_H
#define SCM_STACK_FRAME_H

#include <ostream>
#include <stack>

#include "scheme/common.h"
#include "scheme/instruction.h"
#include "scheme/object.h"
#include "scheme/platform.h"
#include "scheme/procedure.h"

namespace scm {
class StackFrame {
  friend class Interpreter;
  DEFINE_DEFAULT_COPYABLE_TYPE(StackFrame);

 private:
  uint64_t id_;
  instr::TargetEntryInstr* target_;
  LocalScope* locals_;
  uword return_address_;

  StackFrame(const uword id, instr::TargetEntryInstr* target, LocalScope* locals, const uword return_address = UNALLOCATED) :
    id_(id),
    target_(target),
    locals_(locals),
    return_address_(return_address) {
    ASSERT(locals);
  }
  StackFrame(uword id, LocalScope* locals, const uword return_address = UNALLOCATED) :
    id_(id),
    target_(nullptr),
    locals_(locals),
    return_address_(return_address) {
    ASSERT(locals_);
  }

  void SetReturnAddress(const uword addr) {
    ASSERT(addr > UNALLOCATED);
  }

 public:
  StackFrame() :
    id_(0),
    target_(nullptr),
    locals_(nullptr),
    return_address_(UNALLOCATED) {}
  ~StackFrame() = default;

  auto GetId() const -> uword {
    return id_;
  }

  auto GetTarget() const -> instr::TargetEntryInstr* {
    return target_;
  }

  inline auto HasTarget() const -> bool {
    return GetTarget() != nullptr;
  }

  inline auto IsNativeFrame() const -> bool {
    return GetTarget() == nullptr;
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

  inline auto GetReturnInstr() const -> instr::Instruction* {
    return ((instr::Instruction*)GetReturnAddressPointer());  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  friend auto operator<<(std::ostream& stream, const StackFrame& rhs) -> std::ostream& {
    stream << "StackFrame(";
    stream << "target=" << rhs.GetTarget() << ", ";
    if (rhs.HasReturnAddress())
      stream << "result=" << rhs.GetReturnInstr()->ToString() << ", ";
    stream << "locals=" << rhs.GetLocals()->ToString();
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
