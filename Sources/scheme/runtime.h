#ifndef SCM_RUNTIME_H
#define SCM_RUNTIME_H

#include <gflags/gflags_declare.h>

#include <stack>
#include <utility>

#include "scheme/common.h"
#include "scheme/flags.h"
#include "scheme/flow_graph.h"
#include "scheme/instruction.h"
#include "scheme/local_scope.h"

namespace scm {
DECLARE_bool(kernel);
DECLARE_string(module_dir);

namespace proc {
class import;
class exit;
}  // namespace proc

class ExecutionStack {
  using Stack = std::stack<Type*>;
  DEFINE_NON_COPYABLE_TYPE(ExecutionStack);

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

 public:
  virtual ~ExecutionStack() = default;

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
};

class Module;
class Runtime : public ExecutionStack {
  friend class Repl;
  friend class proc::import;
  friend class proc::exit;
  friend class Interpreter;
  friend class RuntimeScopeScope;

  using ModuleList = std::vector<Module*>;
  DEFINE_NON_COPYABLE_TYPE(Runtime);

 private:
  LocalScope* scope_ = nullptr;
  Instruction* current_ = nullptr;
  ModuleList modules_{};
  bool running_ = false;

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

  inline void PushScope() {
    DLOG(INFO) << "pushing scope....";
    ASSERT(HasScope());
    const auto new_scope = LocalScope::New(GetScope());
    ASSERT(new_scope);
    SetScope(new_scope);
  }

  inline void PopScope() {
    DLOG(INFO) << "popping scope....";
    ASSERT(HasScope());
    const auto curr_scope = GetScope();
    ASSERT(curr_scope);
    ASSERT(curr_scope->HasParent());
    SetScope(curr_scope->GetParent());
    ASSERT(HasScope());
  }

  inline void SetRunning(const bool rhs = true) {
    running_ = rhs;
  }

  inline void StopRunning() {
    return SetRunning(false);
  }

  void LoadKernelModule();

 public:
  static auto CreateInitScope() -> LocalScope*;

 protected:
  explicit Runtime(LocalScope* init_scope = CreateInitScope());
  auto StoreSymbol(Symbol* symbol, Type* value) -> bool;

  auto DefineSymbol(Symbol* symbol, Type* value) -> bool;
  auto LookupSymbol(Symbol* symbol, Type** result) -> bool;
  auto CallProcedure(Procedure* procedure) -> bool;

  auto ImportModule(Module* module) -> bool;
  auto ImportModule(Symbol* symbol) -> bool;

  inline auto ImportModule(const std::string& name) -> bool {
    return ImportModule(Symbol::New(name));
  }

 public:
  ~Runtime();

  auto GetScope() const -> LocalScope* {
    return scope_;
  }

  inline auto HasScope() const -> bool {
    return GetScope() != nullptr;
  }

  auto IsRunning() const -> bool {
    return running_;
  }

  auto Execute(GraphEntryInstr* entry) -> Type*;

 private:
  static auto Eval(GraphEntryInstr* graph_entry) -> Type*;
  static inline auto Eval(FlowGraph* flow_graph) -> Type* {
    ASSERT(flow_graph);
    return Eval(flow_graph->GetEntry());
  }

 public:
  static inline auto New(LocalScope* init_scope = CreateInitScope()) -> Runtime* {
    return new Runtime(init_scope);
  }

  static auto Eval(const std::string& expr) -> Type*;

 public:
  static void Init();
};

auto GetRuntime() -> Runtime*;

inline auto HasRuntime() -> bool {
  return GetRuntime() != nullptr;
}

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
