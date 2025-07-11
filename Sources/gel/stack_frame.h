#ifndef GEL_STACK_FRAME_H
#define GEL_STACK_FRAME_H

#include <concepts>
#include <ostream>
#include <stack>
#include <type_traits>
#include <variant>

#include "gel/common.h"
#include "gel/compiled_code.h"
#include "gel/disassembler.h"
#include "gel/instruction.h"
#include "gel/native_procedure.h"
#include "gel/object.h"
#include "gel/operation_stack.h"
#include "gel/platform.h"
#include "gel/procedure.h"
#include "gel/type_traits.h"
#include "gel/util.h"

namespace gel {

template <class T>
concept StackFrameTarget = HasCompiledCode<T> || std::same_as<T, NativeProcedure>;

class StackFrame {  // TODO: extend Object
  friend class Runtime;
  friend class CallStack;
  friend class Collector;
  friend class Interpreter;
  friend class NativeProcedureEntry;
  DEFINE_DEFAULT_COPYABLE_TYPE(StackFrame);

 private:
  uint64_t id_ = 0;
  Object* target_ = UNALLOCATED;
  LocalScope* locals_ = UNALLOCATED;
  uword return_address_ = UNALLOCATED;
  OperationStack stack_{};

  StackFrame(const uword id, Object* target, LocalScope* locals, const uword return_address = UNALLOCATED) :
    id_(id),
    target_(target),
    locals_(locals),
    return_address_(return_address) {
    ASSERT(target_);
    ASSERT(locals);
  }

  void SetReturnAddress(const uword addr) {
    ASSERT(addr > UNALLOCATED);
    return_address_ = addr;
  }

 public:
  StackFrame() = default;
  template <StackFrameTarget Target>
  StackFrame(const uword id, Target* target, LocalScope* locals, const uword return_address = UNALLOCATED) :
    id_(id),
    target_(target),
    locals_(locals),
    return_address_(return_address) {
    ASSERT(target_);
    ASSERT(locals);
  }
  ~StackFrame() = default;

  auto stack() const -> const OperationStack& {
    return stack_;
  }

  auto GetOperationStack() -> OperationStack* {
    return &stack_;
  }

  auto GetStackTop() -> Object* {
    return !stack_.IsEmpty() ? stack_.top() : Nil::Get();
  }

  auto GetId() const -> uword {
    return id_;
  }

  auto GetTarget() const -> Object* {
    return target_;
  }

  auto IsScriptFrame() const -> bool {
    return GetTarget()->IsScript();
  }

  auto IsLambdaFrame() const -> bool {
    return GetTarget()->IsLambda();
  }

  auto IsNativeFrame() const -> bool {
    return GetTarget()->IsNativeProcedure();
  }

  auto IsInitFrame() const -> bool {
    return GetTarget()->IsConstructor();
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

  inline auto GetReturnObjectPointer() const -> Object* {
    return (Object*)GetReturnAddress();  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  auto ToString() const -> std::string;
  auto GetTargetName() const -> std::string;

  auto VisitAllPointerPointers(PointerPointerVisitor* vis) -> bool;
  auto VisitAllPointerPointers(const std::function<bool(Pointer**)>& vis) -> bool;

  friend auto operator<<(std::ostream& stream, const StackFrame& rhs) -> std::ostream& {
    return stream << rhs.ToString();
  }

  auto operator==(const StackFrame& rhs) const -> bool {
    return GetId() == rhs.GetId() && GetReturnAddress() == rhs.GetReturnAddress();
  }
};

class StackFrameLogger : public PrettyLogger {
  using Severity = google::LogSeverity;
  DEFINE_NON_COPYABLE_TYPE(StackFrameLogger);

 private:
  bool recursive_;

 public:
  explicit StackFrameLogger(const char* file, const int line, const Severity severity, const int indent,
                            const bool recursive) :
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
  StackFrame* enter_{};
  StackFrame* exit_{};
  TargetInfoCallback target_info_;

 public:
  StackFrameGuardBase(TargetInfoCallback target_info);
  virtual ~StackFrameGuardBase();
};

template <StackFrameTarget Target>
class StackFrameGuard : public StackFrameGuardBase {
  DEFINE_NON_COPYABLE_TYPE(StackFrameGuard);

 private:
  Target* target_;

  static inline auto PrintTargetInfo(Target* target) -> TargetInfoCallback {
    ASSERT(target);
    return [target]() {
      ASSERT(target);
      // TODO:
      //  LOG(ERROR) << "Target: " << ((void*)target) << " ; " << target->ToString();
      //  if (!target->IsNativeProcedure()) {
      //    LOG(ERROR) << "Target Instructions:";
      //    Disassembler disassembler(std::cerr, LocalScope::New());  // TODO: need to get local scope
      //    disassembler.Disassemble(target->GetCode(), target->GetType()->GetName()->Get().c_str());
      //  }
    };
  }

 public:
  explicit StackFrameGuard(Target* target) :
    StackFrameGuardBase(PrintTargetInfo(target)),
    target_(target) {
    ASSERT(target_);
  }
  ~StackFrameGuard() override = default;

  auto GetTarget() const -> Target* {
    return target_;
  }
};

}  // namespace gel

#endif  // GEL_STACK_FRAME_H
