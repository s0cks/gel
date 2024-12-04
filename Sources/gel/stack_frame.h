#ifndef GEL_STACK_FRAME_H
#define GEL_STACK_FRAME_H

#include <ostream>
#include <stack>
#include <type_traits>
#include <variant>

#include "gel/common.h"
#include "gel/instruction.h"
#include "gel/native_procedure.h"
#include "gel/object.h"
#include "gel/platform.h"
#include "gel/procedure.h"
#include "gel/type_traits.h"
#include "gel/util.h"

namespace gel {
class StackFrame {
  friend class Interpreter;
  DEFINE_DEFAULT_COPYABLE_TYPE(StackFrame);

 public:
  using TargetVariant = std::variant<ir::TargetEntryInstr*, NativeProcedure*>;

 private:
  uint64_t id_;
  TargetVariant target_;
  LocalScope* locals_;
  uword return_address_;

  StackFrame(const uword id, ir::TargetEntryInstr* target, LocalScope* locals, const uword return_address = UNALLOCATED) :
    id_(id),
    target_(target),
    locals_(locals),
    return_address_(return_address) {
    ASSERT(locals);
  }
  StackFrame(uword id, NativeProcedure* target, LocalScope* locals, const uword return_address = UNALLOCATED) :
    id_(id),
    target_(target),
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
    target_(),
    locals_(nullptr),
    return_address_(UNALLOCATED) {}
  ~StackFrame() = default;

  auto GetId() const -> uword {
    return id_;
  }

  auto GetTarget() const -> const TargetVariant& {
    return target_;
  }

  inline auto IsTargetEntryInstr() const -> bool {
    return std::holds_alternative<ir::TargetEntryInstr*>(GetTarget());
  }

  auto GetTargetAsTargetEntryInstr() const -> ir::TargetEntryInstr* {
    ASSERT(IsTargetEntryInstr());
    return std::get<ir::TargetEntryInstr*>(GetTarget());
  }

  inline auto IsNativeFrame() const -> bool {
    return std::holds_alternative<NativeProcedure*>(GetTarget());
  }

  auto GetTargetAsNativeProcedure() const -> NativeProcedure* {
    ASSERT(IsNativeFrame());
    return std::get<NativeProcedure*>(GetTarget());
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

  inline auto GetReturnInstr() const -> ir::Instruction* {
    return ((ir::Instruction*)GetReturnAddressPointer());  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  auto ToString() const -> std::string;
  friend auto operator<<(std::ostream& stream, const StackFrame& rhs) -> std::ostream& {
    return stream << rhs.ToString();
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

class StackFrameLogger : public PrettyLogger {
  using Severity = google::LogSeverity;
  DEFINE_NON_COPYABLE_TYPE(StackFrameLogger);

 private:
  bool recursive_;

 public:
  explicit StackFrameLogger(const char* file, const int line, const Severity severity, const int indent, const bool recursive) :
    PrettyLogger(file, line, severity, indent),
    recursive_(recursive) {}
  ~StackFrameLogger() override = default;

  auto IsRecursive() const -> bool {
    return recursive_;
  }

  void Visit(const StackFrame& frame);

 public:
  template <const Severity S = google::INFO, const int Indent = 0, const bool IsRecursive = true>
  static inline void LogStackFrame(const char* file, const int line, const StackFrame& frame) {
    StackFrameLogger logger(file, line, S, Indent, IsRecursive);
    return logger.Visit(frame);
  }
};

class StackFrameGuardBase {
  DEFINE_NON_COPYABLE_TYPE(StackFrameGuardBase);

 public:
  using TargetInfoCallback = std::function<void()>;

 private:
  std::optional<StackFrame> enter_{};
  std::optional<StackFrame> exit_{};
  TargetInfoCallback target_info_;

 public:
  StackFrameGuardBase(TargetInfoCallback target_info);
  virtual ~StackFrameGuardBase();
};

template <typename T, typename = typename std::enable_if_t<gel::is_executable<T>::value && gel::has_to_string<T>::value>>
class StackFrameGuard : public StackFrameGuardBase {
  DEFINE_NON_COPYABLE_TYPE(StackFrameGuard);

 private:
  T* target_;

  static inline auto PrintTargetInfo(T* target) -> TargetInfoCallback {
    ASSERT(target);
    return [target]() {
      ASSERT(target);
      LOG(ERROR) << "Target: " << ((void*)target) << " ; " << target->ToString();
      if (!target->IsNativeProcedure()) {
        LOG(ERROR) << "Target Instructions:";
        InstructionLogger::Log(target);
      }
    };
  }

 public:
  explicit StackFrameGuard(T* target) :
    StackFrameGuardBase(PrintTargetInfo(target)),
    target_(target) {
    ASSERT(target_);
  }
  ~StackFrameGuard() override = default;

  auto GetTarget() const -> T* {
    return target_;
  }
};

}  // namespace gel

#endif  // GEL_STACK_FRAME_H
