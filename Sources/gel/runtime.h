#ifndef GEL_RUNTIME_H
#define GEL_RUNTIME_H

#include <gflags/gflags_declare.h>

#include <rpp/observers/dynamic_observer.hpp>
#include <rpp/sources/fwd.hpp>
#include <stack>
#include <type_traits>
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
#include "gel/type_traits.h"

namespace gel {
DECLARE_bool(log_script_instrs);

class Module;
class Runtime {
  friend class proc::import;
  friend class proc::exit;
  friend class proc::format;  // TODO: remove
  friend class proc::rx_take_while;
#ifdef GEL_DEBUG
  friend class proc::gel_get_frame;
  friend class proc::gel_print_st;
#endif  // GEL_DEBUG
  friend class Repl;
  friend class Lambda;
  friend class Module;
  friend class Interpreter;
  friend class Interpreter;
  friend class RuntimeTest;
  friend class ModuleLoader;
  friend class DirModuleLoader;
  friend class NativeProcedure;
  friend class RuntimeScopeScope;
  friend class NativeProcedureEntry;
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

  inline auto GetOperationStack() -> OperationStack* {
    ASSERT(!stack_.empty());
    return stack_.top().GetOperationStack();
  }

  template <class E>
  inline void CallWithNArgs(E* exec, const uword num_args, std::enable_if_t<gel::is_executable<E>::value>* = nullptr) {
    ASSERT(exec);
    ASSERT(num_args >= 0);
    const auto stack = GetOperationStack();
    ASSERT(stack);
    std::vector<Object*> args{};
    word remaining = static_cast<word>(num_args);
    for (const auto& arg : exec->GetArgs()) {
      if (arg.IsVararg()) {
        while (remaining > 0) {
          const auto value = stack->Pop();
          ASSERT(value);
          args.push_back((*value));
          remaining--;
        }
        break;
      } else if (remaining > 0) {
        const auto value = stack->Pop();
        ASSERT(value);
        args.push_back((*value));
        remaining--;
        continue;
      }
      ASSERT(remaining <= 0);
      if (arg.IsOptional()) {
        remaining--;
        continue;
      }
      LOG(FATAL) << arg << " is not optional.";
    }
    std::ranges::reverse(std::begin(args), std::end(args));
    while (remaining < 0) {
      args.push_back(Null());
      remaining++;
    }
    return Call(exec, args);
  }

  void Call(NativeProcedure* native, const ObjectList& args = {});
  void Call(Lambda* lambda, const ObjectList& args = {});
  void Call(Script* script, const ObjectList& args = {});

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
  auto PushStackFrame(Script* script, LocalScope* locals) -> const StackFrame&;
  auto PushStackFrame(Lambda* lambda, LocalScope* locals) -> const StackFrame&;
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
  auto Import(Module* module) -> bool;
  auto Import(Symbol* symbol, LocalScope* scope) -> bool;

  inline auto Import(const std::string& name, LocalScope* scope) -> bool {
    return Import(Symbol::New(name), scope);
  }

  // Stack
  inline void PushError(Error* error) {
    ASSERT(error);
    const auto stack = GetOperationStack();
    ASSERT(stack);
    return stack->Push(error);
  }

  inline void PushError(const std::string& message) {
    ASSERT(!message.empty());
    return PushError(Error::New(message));
  }

 public:
  ~Runtime() = default;

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

  auto GetCurrentStackFrame() const -> const StackFrame& {
    return stack_.top();
  }

  template <class E>
  inline auto CallPop(E* exec, const ObjectList& args = {}, std::enable_if_t<gel::is_executable<E>::value>* = nullptr)
      -> Object* {
    ASSERT(exec);
    Call(exec, args);
    if (!stack_.empty())
      return GetOperationStack()->PopOr(Null());
    return Null();  // TODO: don't implicitly return null
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

auto GetRuntime() -> Runtime*;

inline auto HasRuntime() -> bool {
  return GetRuntime() != nullptr;
}

auto GetGelPathEnvVar() -> const EnvironmentVariable&;

static inline auto LsGelPath() -> rx::dynamic_observable<std::filesystem::path> {
  std::unordered_set<std::string> paths;
  const auto home = GetHomeEnvVar().value();
  paths.insert(fmt::format("{}/lib", (*home)));
  const auto path = GetGelPathEnvVar();
  if (path)
    Split(*(path.value()), ';', paths);
  return rx::source::from_iterable(paths) | rx::operators::flat_map([](std::string p) {
           return rx::source::create<std::filesystem::path>([p](const auto& s) {
             for (const auto& entry : std::filesystem::directory_iterator(p)) {
               s.on_next(entry);
             }
             s.on_completed();
           });
         });
}

static inline auto SearchGelPath(const std::string& filename) -> rx::dynamic_observable<std::filesystem::path> {
  std::unordered_set<std::string> paths;
  const auto home = GetHomeEnvVar().value();
  paths.insert(fmt::format("{}/lib", (*home)));
  const auto path = GetGelPathEnvVar();
  if (path)
    Split(*(path.value()), ';', paths);
  return rx::source::create<std::filesystem::path>([&paths, filename](const auto& s) {
    for (const auto& dir : paths) {
      DLOG(INFO) << "searching: " << dir << "....";
      for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (!std::filesystem::is_regular_file(entry)) {
          DVLOG(1000) << "skipping: " << entry.path();
          continue;
        }
        const auto& path = entry.path();
        if (path == filename)
          s.on_next(path);
      }
    }
    s.on_completed();
  });
}
}  // namespace gel

#endif  // GEL_RUNTIME_H
