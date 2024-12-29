#ifndef GEL_STACK_FRAME_H
#define GEL_STACK_FRAME_H

#include <ostream>
#include <stack>
#include <type_traits>
#include <variant>

#include "gel/common.h"
#include "gel/disassembler.h"
#include "gel/instruction.h"
#include "gel/native_procedure.h"
#include "gel/object.h"
#include "gel/platform.h"
#include "gel/procedure.h"
#include "gel/type_traits.h"
#include "gel/util.h"

namespace gel {

using Stack = std::stack<Object*>;

class ExecutionStack {
  friend class Runtime;
  friend class StackFrame;
  friend class Interpreter;
  DEFINE_DEFAULT_COPYABLE_TYPE(ExecutionStack);

 private:
  Stack stack_{};

 protected:
  ExecutionStack() = default;

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
  virtual ~ExecutionStack() = default;

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

class StackFrame {
  friend class Runtime;
  friend class Interpreter;
  DEFINE_DEFAULT_COPYABLE_TYPE(StackFrame);

 public:
  using TargetVariant = std::variant<Script*, Lambda*, NativeProcedure*>;

 private:
  uint64_t id_;
  TargetVariant target_;
  LocalScope* locals_;
  uword return_address_;
  ExecutionStack stack_{};

  StackFrame(const uword id, const TargetVariant target, LocalScope* locals, const uword return_address = UNALLOCATED) :
    id_(id),
    target_(target),
    locals_(locals),
    return_address_(return_address) {
    ASSERT(locals);
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

  auto stack() const -> const ExecutionStack& {
    return stack_;
  }

  auto GetExecutionStack() -> ExecutionStack* {
    return &stack_;
  }

  auto GetId() const -> uword {
    return id_;
  }

  auto target() const -> const TargetVariant& {
    return target_;
  }

  auto IsScriptFrame() const -> bool {
    return std::holds_alternative<Script*>(target());
  }

  auto GetScript() const -> Script* {
    return std::get<Script*>(target());
  }

  auto IsLambdaFrame() const -> bool {
    return std::holds_alternative<Lambda*>(target());
  }

  auto GetLambda() const -> Lambda* {
    return std::get<Lambda*>(target());
  }

  auto IsNativeFrame() const -> bool {
    return std::holds_alternative<NativeProcedure*>(target());
  }

  auto GetNativeProcedure() const -> NativeProcedure* {
    return std::get<NativeProcedure*>(target());
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

  auto GetTargetName() const -> std::string;
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
