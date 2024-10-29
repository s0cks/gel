#ifndef SCM_INTERPRETER_H
#define SCM_INTERPRETER_H

#include "scheme/instruction.h"

namespace scm {
class Runtime;
class Interpreter : public InstructionVisitor {
  friend class Runtime;
  DEFINE_NON_COPYABLE_TYPE(Interpreter);

 private:
  Runtime* runtime_;
  Instruction* current_ = nullptr;

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

 public:
  ~Interpreter() = default;

  auto GetRuntime() const -> Runtime* {
    return runtime_;
  }

  void Run(GraphEntryInstr* entry);
#define DECLARE_VISIT(Name) auto Visit##Name(Name* instr) -> bool override;
  FOR_EACH_INSTRUCTION(DECLARE_VISIT)
#undef DECLARE_VISIT
};
}  // namespace scm

#endif  // SCM_INTERPRETER_H
