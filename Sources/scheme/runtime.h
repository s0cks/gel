#ifndef SCM_RUNTIME_H
#define SCM_RUNTIME_H

#include <stack>

#include "scheme/common.h"
#include "scheme/flow_graph.h"
#include "scheme/instruction.h"
#include "scheme/local_scope.h"

namespace scm {
class Runtime : private InstructionVisitor {
  friend class RuntimeScopeScope;
  using Stack = std::stack<Type*>;
  DEFINE_NON_COPYABLE_TYPE(Runtime);

 private:
  LocalScope* scope_ = nullptr;
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

  inline void SetScope(LocalScope* scope) {
    ASSERT(scope);
    scope_ = scope;
  }

  inline void SetStack(const Stack& rhs) {
    ASSERT(!rhs.empty());
    stack_ = rhs;
  }

  static auto CreateInitScope() -> LocalScope*;

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
  Runtime(LocalScope* init_scope = CreateInitScope()) {
    SetScope(init_scope);
  }
  ~Runtime() {
    if (HasScope())
      delete scope_;
  }

  auto GetScope() const -> LocalScope* {
    return scope_;
  }

  inline auto HasScope() const -> bool {
    return GetScope() != nullptr;
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

class RuntimeScopeScope {
  DEFINE_NON_COPYABLE_TYPE(RuntimeScopeScope);

 private:
  Runtime* runtime_;

 public:
  explicit RuntimeScopeScope(Runtime* runtime) :
    runtime_(runtime) {
    ASSERT(runtime);
    const auto new_scope = LocalScope::New(GetRuntime()->GetScope());
    ASSERT(new_scope);
    GetRuntime()->SetScope(new_scope);
  }
  ~RuntimeScopeScope() {
    const auto curr_scope = GetRuntime()->GetScope();
    ASSERT(curr_scope);
    const auto next_scope = curr_scope->GetParent();
    ASSERT(next_scope);
    GetRuntime()->SetScope(next_scope);
  }

  inline auto GetRuntime() const -> Runtime* {
    return runtime_;
  }

  inline auto GetCurrentScope() const -> LocalScope* {
    return GetRuntime()->GetScope();
  }

  inline auto GetParentScope() const -> LocalScope* {
    return GetCurrentScope()->GetParent();
  }

  inline auto HasParentScope() const -> bool {
    return GetParentScope() != nullptr;
  }

  inline auto operator->() const -> LocalScope* {
    return GetCurrentScope();
  }
};
}  // namespace scm

#endif  // SCM_RUNTIME_H
