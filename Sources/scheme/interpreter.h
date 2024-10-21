#ifndef SCM_INTERPRETER_H
#define SCM_INTERPRETER_H

#include <stack>

#include "scheme/common.h"
#include "scheme/environment.h"
#include "scheme/flow_graph.h"
#include "scheme/instruction.h"

namespace scm {
class Interpreter : private InstructionVisitor {
  DEFINE_NON_COPYABLE_TYPE(Interpreter);

 private:
  std::stack<Datum*> stack_{};
  Environment* env_ = nullptr;

  inline void Push(Datum* value) {
    ASSERT(value);
    stack_.push(value);
  }

  inline auto Pop() -> Datum* {
    if (stack_.empty())
      return nullptr;
    const auto next = stack_.top();
    stack_.pop();
    return next;
  }

  void ExecuteInstr(Instruction* instr);
  void StoreSymbol(Symbol* symbol, Datum* value);
  auto LoadSymbol(Symbol* symbol) -> bool;

  void SetEnvironment(Environment* env) {
    ASSERT(env);
    env_ = env;
  }

#define DECLARE_VISIT(Name) auto Visit##Name(Name* instr) -> bool override;
  FOR_EACH_INSTRUCTION(DECLARE_VISIT)
#undef DECLARE_VISIT

 public:
  Interpreter() {
    SetEnvironment(Environment::New());
  }
  ~Interpreter() {
    delete env_;
  }

  auto GetEnvironment() const -> Environment* {
    return env_;
  }

  auto Execute(EntryInstr* entry) -> Datum*;

 public:
  static inline auto Eval(EntryInstr* instr) -> Datum* {
    ASSERT(instr);
    Interpreter interpreter;
    return interpreter.Execute(instr);
  }

  static inline auto Eval(FlowGraph* flow_graph) -> Datum* {
    ASSERT(flow_graph);
    return Eval(flow_graph->GetEntry());
  }
};
}  // namespace scm

#endif  // SCM_INTERPRETER_H
