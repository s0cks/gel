#ifndef SCM_RUNTIME_H
#define SCM_RUNTIME_H

#include <gflags/gflags_declare.h>

#include <stack>
#include <utility>

#include "scheme/common.h"
#include "scheme/error.h"
#include "scheme/flags.h"
#include "scheme/flow_graph.h"
#include "scheme/instruction.h"
#include "scheme/interpreter.h"
#include "scheme/local_scope.h"
#include "scheme/native_procedure.h"
#include "scheme/natives.h"
#include "scheme/object.h"
#include "scheme/stack_frame.h"

namespace scm {
DECLARE_bool(kernel);
DECLARE_string(module_dir);
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
#ifdef SCM_DEBUG
  friend class proc::scm_get_frame;
#endif  // SCM_DEBUG
  friend class Repl;
  friend class Lambda;
  friend class Interpreter;
  friend class NativeProcedure;
  friend class RuntimeScopeScope;
  friend class RuntimeStackIterator;

  using ModuleList = std::vector<Module*>;
  DEFINE_NON_COPYABLE_TYPE(Runtime);

 private:
  LocalScope* init_scope_;
  LocalScope* scope_ = nullptr;
  std::vector<Script*> scripts_{};
  bool running_ = false;
  bool executing_ = false;
  Interpreter interpreter_;

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

  inline void SetRunning(const bool rhs = true) {
    running_ = rhs;
  }

  inline void StopRunning() {
    return SetRunning(false);
  }

  auto GetStackFrames() const -> const std::stack<StackFrame>& {
    return interpreter_.stack_;
  }

  template <class Native>
  static inline auto RegisterNative(LocalScope* scope) -> std::pair<Symbol*, NativeProcedure*> {
    ASSERT(scope);
    Native::Init();
    const auto procedure = Native::Get();
    ASSERT(procedure);
    const auto symbol = procedure->GetSymbol();
    LOG_IF(FATAL, !scope->Add(symbol, procedure)) << "failed to register: " << procedure->ToString();
    return {symbol, procedure};
  }

  void Call(instr::TargetEntryInstr* target, LocalScope* locals);
  void Call(NativeProcedure* native, const ObjectList& args);
  void Call(Lambda* lambda, const ObjectList& args);

 public:  // TODO: reduce visibility
  void LoadKernelModule();
  inline void Call(Procedure* procedure, const ObjectList& args) {
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
  auto Apply(Procedure* proc, const std::vector<Object*>& args) -> Object*;
  auto Import(Script* module) -> bool;
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
  ~Runtime() override;

  auto IsRunning() const -> bool {
    return running_;
  }

  auto GetCurrentFrame() -> StackFrame* {
    return interpreter_.GetCurrentStackFrame();
  }

  auto HasFrame() const -> bool {
    return interpreter_.HasStackFrame();
  }

  auto GetGlobalScope() const -> LocalScope* {
    return scope_;
  }

  auto GetInitScope() const -> LocalScope* {
    return init_scope_;
  }

  auto GetCurrentScope() -> LocalScope* {
    const auto frame = GetCurrentFrame();
    return frame ? frame->GetLocals() : GetInitScope();
  }

 public:
  static auto CreateInitScope() -> LocalScope*;
  static auto Eval(const std::string& expr) -> Object*;
  static auto Exec(Script* script) -> Object*;

  static inline auto New(LocalScope* init_scope = CreateInitScope()) -> Runtime* {
    return new Runtime(init_scope);
  }

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
}  // namespace scm

#endif  // SCM_RUNTIME_H
