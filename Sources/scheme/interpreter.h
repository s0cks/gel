#ifndef SCM_INTERPRETER_H
#define SCM_INTERPRETER_H

#include <stack>

#include "scheme/common.h"
#include "scheme/environment.h"
#include "scheme/flow_graph.h"
#include "scheme/instruction.h"
#include "scheme/state.h"

namespace scm {
class Interpreter : private InstructionVisitor {
  DEFINE_NON_COPYABLE_TYPE(Interpreter);

 private:
  State* state_ = nullptr;
  Instruction* current_ = nullptr;

  inline void SetState(State* state) {
    ASSERT(state);
    state_ = state;
  }

  void ExecuteInstr(Instruction* instr);
  void StoreSymbol(Symbol* symbol, Type* value);
  auto LoadSymbol(Symbol* symbol) -> bool;

  auto DefineSymbol(Symbol* symbol, Type* value) -> bool;
  auto LookupSymbol(Symbol* symbol, Type** result) -> bool;

  inline void Push(Type* value) {
    ASSERT(value);
    const auto state = GetState();
    ASSERT(state);
    state->Push(value);
  }

  inline auto Pop() -> Type* {
    const auto state = GetState();
    ASSERT(state);
    return state->Pop();
  }

  inline void SetCurrentInstr(Instruction* instr) {
    ASSERT(instr);
    current_ = instr;
  }

  inline auto GetCurrentInstr() const -> Instruction* {
    return current_;
  }

  inline auto HasCurrentInstr() const -> bool {
    return GetCurrentInstr() != nullptr;
  }

#define DECLARE_VISIT(Name) auto Visit##Name(Name* instr) -> bool override;
  FOR_EACH_INSTRUCTION(DECLARE_VISIT)
#undef DECLARE_VISIT

 public:
  Interpreter();
  ~Interpreter();

  auto GetState() const -> State* {
    return state_;
  }

  inline auto HasState() const -> bool {
    return GetState() != nullptr;
  }

  auto Execute(EntryInstr* entry) -> Type*;

 public:
  static inline auto Eval(EntryInstr* instr) -> Type* {
    ASSERT(instr);
    Interpreter interpreter;
    return interpreter.Execute(instr);
  }

  static inline auto Eval(FlowGraph* flow_graph) -> Type* {
    ASSERT(flow_graph);
    return Eval(flow_graph->GetEntry());
  }
};
}  // namespace scm

#endif  // SCM_INTERPRETER_H
