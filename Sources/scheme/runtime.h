#ifndef SCM_RUNTIME_H
#define SCM_RUNTIME_H

#include <stack>

#include "scheme/common.h"
#include "scheme/environment.h"
#include "scheme/flow_graph.h"
#include "scheme/instruction.h"

namespace scm {
class Runtime : private InstructionVisitor {
  friend class RuntimeEnvScope;
  using Stack = std::stack<Type*>;
  DEFINE_NON_COPYABLE_TYPE(Runtime);

 private:
  Environment* env_ = nullptr;
  Instruction* current_ = nullptr;
  Stack stack_{};

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

  inline void SetEnv(Environment* env) {
    ASSERT(env);
    env_ = env;
  }

  inline void SetStack(const Stack& rhs) {
    ASSERT(!rhs.empty());
    stack_ = rhs;
  }

 protected:
  void ExecuteInstr(Instruction* instr);
  void StoreSymbol(Symbol* symbol, Type* value);
  auto LoadSymbol(Symbol* symbol) -> bool;
  auto DefineSymbol(Symbol* symbol, Type* value) -> bool;
  auto LookupSymbol(Symbol* symbol, Type** result) -> bool;
  auto CallProcedure(Procedure* procedure) -> bool;
#define DECLARE_VISIT(Name) auto Visit##Name(Name* instr) -> bool override;
  FOR_EACH_INSTRUCTION(DECLARE_VISIT)
#undef DECLARE_VISIT

 public:
  Runtime(Environment* env = Environment::New());
  ~Runtime();

  auto GetEnv() const -> Environment* {
    return env_;
  }

  auto GetStack() const -> const Stack& {
    return stack_;
  }

  auto Pop() -> std::optional<Stack::value_type> {
    if (stack_.empty())
      return std::nullopt;
    const auto next = stack_.top();
    ASSERT(next);
    stack_.pop();
    return {next};
  }

  void Push(Stack::value_type value) {
    ASSERT(value);
    stack_.push(value);
  }

  auto Execute(EntryInstr* entry) -> Type*;

 public:
  static inline auto Eval(EntryInstr* instr) -> Type* {
    ASSERT(instr);
    Runtime interpreter;
    return interpreter.Execute(instr);
  }

  static inline auto Eval(FlowGraph* flow_graph) -> Type* {
    ASSERT(flow_graph);
    return Eval(flow_graph->GetEntry());
  }
};

class RuntimeEnvScope {
  DEFINE_NON_COPYABLE_TYPE(RuntimeEnvScope);

 private:
  Runtime* runtime_;
  Environment* previous_ = nullptr;

 public:
  explicit RuntimeEnvScope(Runtime* runtime, Environment* env = nullptr) :
    runtime_(runtime) {
    ASSERT(runtime);
    previous_ = GetRuntime()->GetEnv();
    GetRuntime()->SetEnv(env ? env : Environment::New(previous_));
  }
  ~RuntimeEnvScope() {
    GetRuntime()->SetEnv(GetPreviousEnv());
  }

  auto GetRuntime() const -> Runtime* {
    return runtime_;
  }

  auto GetPreviousEnv() const -> Environment* {
    return previous_;
  }

  auto GetCurrentEnv() const -> Environment* {
    return GetRuntime()->GetEnv();
  }
};
}  // namespace scm

#endif  // SCM_RUNTIME_H
