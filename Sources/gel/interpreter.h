#ifndef GEL_INTERPRETER_H
#define GEL_INTERPRETER_H

#include <type_traits>

#include "gel/common.h"
#include "gel/instruction.h"
#include "gel/platform.h"
#include "gel/stack_frame.h"
#include "gel/type_traits.h"

namespace gel {
class Runtime;
class Interpreter : public InstructionVisitor {
  friend class Runtime;
  DEFINE_NON_COPYABLE_TYPE(Interpreter);

 private:
  Runtime* runtime_;
  Instruction* current_ = nullptr;
  std::stack<StackFrame> stack_{};

 protected:
  explicit Interpreter(Runtime* runtime);

  inline void SetCurrentInstr(Instruction* instr) {
    current_ = instr;
  }

  inline auto GetCurrentInstr() const -> Instruction* {
    return current_;
  }

  inline auto HasCurrentInstr() const -> bool {
    return GetCurrentInstr() != nullptr;
  }

  auto ExecuteInstr(Instruction* instr) -> bool;

  auto GetCurrentStackFrame() -> StackFrame* {
    return stack_.empty() ? nullptr : &stack_.top();
  }

  auto HasStackFrame() const -> bool {
    return !stack_.empty();
  }

  auto PopStackFrame() -> StackFrame;
  auto PushStackFrame(instr::TargetEntryInstr* target, LocalScope* locals) -> StackFrame*;
  auto PushStackFrame(NativeProcedure* native, LocalScope* locals) -> StackFrame*;

  void Run();

  template <typename E>
  void Execute(E* executable, LocalScope* locals, std::enable_if_t<gel::is_executable<E>::value>* = nullptr) {
    ASSERT(executable && executable->IsCompiled());
    ASSERT(locals);
    DVLOG(1000) << "executing " << executable << " with locals: " << locals;
    const auto target = executable->GetEntry()->GetTarget();
    PushStackFrame(target, locals);
    SetCurrentInstr(target->GetNext());
    Run();
  }

  inline auto Next() -> bool {
    const auto next = HasCurrentInstr() ? GetCurrentInstr()->GetNext() : nullptr;
    SetCurrentInstr(next);
    return true;
  }

  auto PushError(const std::string& message) -> bool;
  auto PushNext(Object* rhs) -> bool;

  inline auto Goto(Instruction* instr) -> bool {
    SetCurrentInstr(instr);
    return true;
  }

  auto GetStackTop() const -> std::optional<Object*>;

  inline auto IsStackTopInstanceOf(Class* rhs) const -> bool {
    ASSERT(rhs);
    const auto stack_top = GetStackTop();
    ASSERT(stack_top);
    return (*stack_top)->GetType()->IsInstanceOf(rhs);
  }

 public:
  ~Interpreter() override = default;

  auto GetRuntime() const -> Runtime* {
    return runtime_;
  }

#define DECLARE_VISIT(Name) auto Visit##Name(Name* instr)->bool override;
  FOR_EACH_INSTRUCTION(DECLARE_VISIT)
#undef DECLARE_VISIT
};
}  // namespace gel

#endif  // GEL_INTERPRETER_H
