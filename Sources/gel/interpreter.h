#ifndef GEL_INTERPRETER_H
#define GEL_INTERPRETER_H

#include <type_traits>

#include "gel/common.h"
#include "gel/instruction.h"
#include "gel/local_scope.h"
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
  Instruction* previous_ = nullptr;
  Instruction* current_ = nullptr;

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

  inline void SetPreviousInstr(Instruction* instr) {
    ASSERT(instr);
    previous_ = instr;
  }

  inline auto GetPreviousInstr() const -> Instruction* {
    return previous_;
  }

  inline auto HasPreviousInstr() const -> bool {
    return GetPreviousInstr() != nullptr;
  }

  auto ExecuteInstr(Instruction* instr) -> bool;
  void Run();

  template <typename E>
  void Execute(E* executable, LocalScope* locals, std::enable_if_t<gel::is_executable<E>::value>* = nullptr) {
    ASSERT(executable && executable->IsCompiled());
    ASSERT(locals);
    DVLOG(1) << "executing " << executable << " with locals: " << locals->ToString();
    const auto target = executable->GetEntry()->GetTarget();
    SetCurrentInstr(target);
    Run();
  }

  inline auto Next() -> bool {
    if (HasCurrentInstr()) {
      SetPreviousInstr(GetCurrentInstr());
      SetCurrentInstr(GetCurrentInstr()->GetNext());
      return HasCurrentInstr();
    }
    SetCurrentInstr(nullptr);
    return false;
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
