#ifndef SCM_INTERPRETER_H
#define SCM_INTERPRETER_H

#include "scheme/common.h"
#include "scheme/instruction.h"
#include "scheme/platform.h"
#include "scheme/stack_frame.h"

namespace scm {
class Runtime;
class Interpreter : public InstructionVisitor {
  friend class Runtime;
  DEFINE_NON_COPYABLE_TYPE(Interpreter);

 private:
  Runtime* runtime_;
  Instruction* current_ = nullptr;
  std::stack<StackFrame> stack_{};

 protected:
  explicit Interpreter(Runtime* runtime) :
    InstructionVisitor(),
    runtime_(runtime) {
    ASSERT(runtime);
  }

  inline void SetCurrentInstr(Instruction* instr) {
    current_ = instr;
  }

  inline auto GetCurrentInstr() const -> Instruction* {
    return current_;
  }

  inline auto HasCurrentInstr() const -> bool {
    return GetCurrentInstr() != nullptr;
  }

  void ExecuteInstr(Instruction* instr);

  auto GetCurrentStackFrame() -> StackFrame* {
    return &stack_.top();
  }

  auto HasStackFrame() const -> bool {
    return !stack_.empty();
  }

  auto PopStackFrame() -> StackFrame;
  auto PushStackFrame(LocalScope* locals) -> StackFrame*;
  void Execute(instr::TargetEntryInstr* target, LocalScope* locals);

  inline auto Next() -> bool {
    const auto next = HasCurrentInstr() ? GetCurrentInstr()->GetNext() : nullptr;
    SetCurrentInstr(next);
    return true;
  }

  auto PushError(const std::string& message) -> bool;

  inline auto Goto(Instruction* instr) -> bool {
    SetCurrentInstr(instr);
    return true;
  }

 public:
  ~Interpreter() = default;

  auto GetRuntime() const -> Runtime* {
    return runtime_;
  }

#define DECLARE_VISIT(Name) auto Visit##Name(Name* instr) -> bool override;
  FOR_EACH_INSTRUCTION(DECLARE_VISIT)
#undef DECLARE_VISIT
};
}  // namespace scm

#endif  // SCM_INTERPRETER_H
