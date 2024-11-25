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

namespace proc {
class import;
class map;
class foreach;
class exit;
class format;
class throw_exc;
class frame;
}  // namespace proc

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
  friend class proc::foreach;
  friend class proc::map;
  friend class proc::frame;
  friend class Repl;
  friend class Lambda;
  friend class Interpreter;
  friend class NativeProcedure;
  friend class RuntimeScopeScope;
  friend class RuntimeStackIterator;

  using ModuleList = std::vector<Module*>;
  DEFINE_NON_COPYABLE_TYPE(Runtime);

 private:
  LocalScope* scope_ = nullptr;
  std::vector<Script*> scripts_{};
  bool running_ = false;
  bool executing_ = false;
  Interpreter interpreter_;

  void Call(instr::TargetEntryInstr* target, LocalScope* locals);
  void Call(NativeProcedure* native, const std::vector<Object*>& args);
  void Call(Lambda* lambda);

  inline void PopN(std::vector<Object*>& result, const uword num, const bool reverse = false) {
    for (auto idx = 0; idx < num; idx++) {
      result.push_back(Pop());
    }
    if (reverse)
      std::ranges::reverse(std::begin(result), std::end(result));
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

 public:
  static auto CreateInitScope() -> LocalScope*;
  void LoadKernelModule();  // TODO: reduce visibility

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
  ~Runtime();

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

  auto GetCurrentScope() -> LocalScope* {
    const auto frame = GetCurrentFrame();
    ASSERT(frame);
    return frame->GetLocals();
  }

 public:
  static inline auto New(LocalScope* init_scope = CreateInitScope()) -> Runtime* {
    return new Runtime(init_scope);
  }

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
}  // namespace scm

#endif  // SCM_RUNTIME_H
