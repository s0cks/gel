#ifndef GEL_RUNTIME_H
#define GEL_RUNTIME_H

#include <gflags/gflags_declare.h>

#include <stack>
#include <utility>

#include "gel/common.h"
#include "gel/error.h"
#include "gel/flags.h"
#include "gel/flow_graph.h"
#include "gel/instruction.h"
#include "gel/interpreter.h"
#include "gel/local_scope.h"
#include "gel/native_procedure.h"
#include "gel/natives.h"
#include "gel/object.h"
#include "gel/stack_frame.h"

namespace gel {
DECLARE_bool(log_script_instrs);

using Stack = std::stack<Object*>;

class ExecutionStack {
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

  inline auto stack() const -> const Stack& {
    return stack_;
  }

 public:
  virtual ~ExecutionStack() = default;

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
};

class Module;
class Runtime : public ExecutionStack {
  friend class proc::import;
  friend class proc::exit;
  friend class proc::format;  // TODO: remove
#ifdef GEL_DEBUG
  friend class proc::gel_get_frame;
#endif  // GEL_DEBUG
  friend class Repl;
  friend class Lambda;
  friend class Interpreter;
  friend class ModuleLoader;
  friend class NativeProcedure;
  friend class RuntimeScopeScope;
  friend class RuntimeStackIterator;
  DEFINE_NON_COPYABLE_TYPE(Runtime);

 private:
  LocalScope* init_scope_;
  LocalScope* curr_scope_;
  Interpreter interpreter_;
  std::stack<StackFrame> stack_{};
  bool executing_ = false;

  inline void SetExecuting(const bool value = true) {
    executing_ = value;
  }

  inline void PopN(std::vector<Object*>& result, const uword num, const bool reverse = false) {
    for (auto idx = 0; idx < num; idx++) {
      result.push_back(Pop());
    }
    if (reverse)
      std::ranges::reverse(std::begin(result), std::end(result));
  }

  inline void CallWithNArgs(Lambda* lambda, const uword num_args) {
    ASSERT(lambda);
    ASSERT(num_args >= 0);
    std::vector<Object*> args{};
    PopN(args, num_args, true);
    ASSERT(num_args == args.size());
    return Call(lambda, args);
  }

  inline void CallWithNArgs(NativeProcedure* native, const uword num_args) {
    ASSERT(native);
    ASSERT(num_args >= 0);
    std::vector<Object*> args{};
    PopN(args, num_args, true);
    ASSERT(num_args == args.size());
    return Call(native, args);
  }

  void Call(NativeProcedure* native, const ObjectList& args);
  void Call(Lambda* lambda, const ObjectList& args);
  void Call(Script* script);

  inline auto CallPop(Lambda* lambda) -> Object* {
    ASSERT(lambda);
    Call(lambda, {});
    return Pop();
  }

  inline auto CallPop(Script* script) -> Object* {
    ASSERT(script);
    Call(script);
    return Pop();
  }

  inline auto PushScope() -> LocalScope* {
    const auto new_scope = LocalScope::New(curr_scope_);
    curr_scope_ = new_scope;
    return new_scope;
  }

  inline void PopScope() {
    if (!curr_scope_)
      return;
    curr_scope_ = curr_scope_->GetParent();
  }

  auto PopStackFrame() -> StackFrame;
  auto PushStackFrame(ir::TargetEntryInstr* target) -> const StackFrame&;
  auto PushStackFrame(NativeProcedure* native, LocalScope* locals) -> const StackFrame&;

 public:  // TODO: reduce visibility
  void LoadKernelModule();
  inline void Call(Procedure* procedure, const ObjectList& args = {}) {
    if (procedure->IsLambda()) {
      return Call(procedure->AsLambda(), args);
    } else if (procedure->IsNativeProcedure()) {
      return Call(procedure->AsNativeProcedure(), args);
    }
    LOG(FATAL) << "invalid Call to " << procedure << " w/ args: " << args.size();  // TODO: fix printing args
  }

 protected:
  explicit Runtime(LocalScope* init_scope = CreateInitScope());
  auto StoreSymbol(Symbol* symbol, Object* value) -> bool;
  auto DefineSymbol(Symbol* symbol, Object* value) -> bool;
  auto LookupSymbol(Symbol* symbol, Object** result) -> bool;
  auto Import(Module* module) -> bool;
  auto Import(Symbol* symbol, LocalScope* scope) -> bool;

  inline auto Import(const std::string& name, LocalScope* scope) -> bool {
    return Import(Symbol::New(name), scope);
  }

  // Stack
  inline void PushError(Error* error) {
    ASSERT(error);
    return Push(error);
  }

  inline void PushError(const std::string& message) {
    ASSERT(!message.empty());
    return PushError(Error::New(message));
  }

 public:
  ~Runtime() override = default;

  auto GetInitScope() const -> LocalScope* {
    return init_scope_;
  }

  auto GetScope() const -> LocalScope* {
    return curr_scope_;
  }

  auto IsExecuting() const -> bool {
    return executing_;
  }

  auto HasStackFrame() const -> bool {
    return !stack_.empty();
  }

  auto GetCurrentStackFrame() -> const StackFrame& {
    return stack_.top();
  }

 private:
  static auto CreateInitScope() -> LocalScope*;

  static inline auto New(LocalScope* init_scope = CreateInitScope()) -> Runtime* {
    return new Runtime(init_scope);
  }

 public:
  static auto Eval(const std::string& expr) -> Object*;
  static auto Exec(Script* script) -> Object*;

 public:
  static void Init();
};

class RuntimeStackIterator {
  DEFINE_NON_COPYABLE_TYPE(RuntimeStackIterator);

 private:
  Stack stack_;

 public:
  RuntimeStackIterator(Runtime* runtime) :
    stack_(runtime->stack()) {}
  ~RuntimeStackIterator() = default;

  auto HasNext() const -> bool {
    return !stack_.empty();
  }

  auto Next() -> Stack::value_type {
    const auto next = stack_.top();
    ASSERT(next);
    stack_.pop();
    return next;
  }
};

auto GetRuntime() -> Runtime*;

inline auto HasRuntime() -> bool {
  return GetRuntime() != nullptr;
}
}  // namespace gel

#endif  // GEL_RUNTIME_H
