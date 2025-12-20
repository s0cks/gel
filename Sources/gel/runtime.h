#ifndef GEL_RUNTIME_H
#define GEL_RUNTIME_H

#include <concepts>
#include <exception>
#include <gflags/gflags_declare.h>
#include <new>
#include <rpp/observers/dynamic_observer.hpp>
#include <rpp/sources/fwd.hpp>
#include <stack>
#include <type_traits>
#include <utility>

#include "call_stack.h"
#include "common.h"
#include "environment.h"
#include "error.h"
#include "flags.h"
#include "flow_graph.h"
#include "instruction.h"
#include "interpreter.h"
#include "local_scope.h"
#include "native_procedure.h"
#include "natives.h"
#include "object.h"
#include "pointer.h"
#include "stack_frame.h"
#include "type_traits.h"

namespace gel {
DECLARE_bool(log_script_instrs);
using ShutdownCallback = std::function<void()>;
using ShutdownCallbackList = std::vector<ShutdownCallback>;

template <typename F, typename... Args>
static inline void invoke_all(const std::vector<F>& funcs, Args... args) {
  std::ranges::for_each(funcs, [=](const ShutdownCallback& cb) {
    cb(args...);
  });
}

template <class T>
concept HasArgs = requires(const T value) {
  { value.GetNumberOfArgs() } -> std::convertible_to<uword>;
  { value.HasArgs() } -> std::convertible_to<bool>;
  { value.GetArgAt((const uint64_t)0) } -> std::convertible_to<Argument*>;
  { value.GetArgs() } -> std::convertible_to<Array<Argument*>*>;
};

template <class T>
concept RuntimeTarget = std::same_as<T, Lambda> || std::same_as<T, NativeFn> || std::same_as<T, Script>;

class Module;
class Runtime {
  friend class CallScope;
  friend class CallStackFrame;
  friend class Collector;
  friend class proc::import;
  friend class proc::exit;
  friend class proc::gel_format;  // TODO: remove
  friend class proc::rx_take_while;
  friend class Repl;
  friend class StackFrameGuardBase;
  friend class Lambda;
  friend class Module;
  friend class Interpreter;
  friend class Interpreter;
  friend class RuntimeTest;
  friend class ModuleLoader;
  friend class NativeFn;
  friend class RuntimeScopeScope;
  friend class NativeFnEntry;
  DEFINE_NON_COPYABLE_TYPE(Runtime);

 private:
  LocalScope* init_scope_;
  LocalScope* curr_scope_;
  bool executing_ = false;
  Object* result_ = nullptr;
  ShutdownCallbackList shutdown_listeners_{};
  bool emptying_task_queue_ = false;
  CallStack call_stack_{};

  inline void SetExecuting(const bool value = true) {
    executing_ = value;
  }

  template <RuntimeTarget Target>
  inline void CallWithNArgs(Target& exec, const uword num_args)
    requires(HasArgs<Target>)
  {
    ASSERT(num_args >= 0);
    const auto stack = GetCallStack()->GetOperationStack();
    ASSERT(stack);
    std::vector<Object*> args{};
    word remaining = static_cast<word>(num_args);
    if (exec.HasArgs()) {
      for (auto idx = 0; idx < exec.GetNumberOfArgs(); idx++) {
        const auto arg = exec.GetArgAt(idx);
        if (arg->IsVararg()) {
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
        if (arg->IsOptional()) {
          remaining--;
          continue;
        }
        LOG(FATAL) << arg->ToString() << " is not optional.";
      }
    }
    std::ranges::reverse(std::begin(args), std::end(args));
    while (remaining < 0) {
      args.push_back(Nil::Get());
      remaining++;
    }
    return Call(exec, args);
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

  inline auto ShouldEmptyTaskQueue() const -> bool {
    return call_stack_.IsEmpty() && !emptying_task_queue_;
  }

  void EmptyTaskQueue();

  auto PopOr(Object* default_value = Nil::Get()) -> Object* {
    ASSERT(default_value);
    if (!GetCallStack().IsEmpty())
      return result_ = GetCallStack()->GetOperationStack()->PopOr(default_value);
    return result_ ? result_ : (result_ = default_value);
  }

 public:  // TODO: reduce visibility
  void LoadKernel();

  template <RuntimeTarget T>
  void Call(T& target, const ObjectList args = {});
  void Call(Fn& target, const ObjectList args = {});

  template <WithInit I>
  inline void InvokeConstructor(I* this_value, const ObjectList& args = {}) {
    ASSERT(this_value);
    if (!this_value->HasInit())
      return;
    ObjectList invoke_args = {
        this_value,
    };
    invoke_args.insert(std::end(invoke_args), std::begin(args), std::end(args));
    return Call(*(this_value->GetInit()), invoke_args);
  }

 protected:
  explicit Runtime(LocalScope* init_scope = CreateInitScope());
  auto Import(Module* module) -> bool;
  auto Import(Symbol* symbol, LocalScope* scope) -> bool;
  auto ImportModule(const std::string& name) -> bool;

  inline auto GetCallStack() -> CallStack& {
    return call_stack_;
  }

  inline auto Import(const std::string& name, LocalScope* scope) -> bool {
    return Import(Symbol::New(name), scope);
  }

  inline void PushError(Error* error) {
    ASSERT(error);
    const auto stack = GetCallStack()->GetOperationStack();
    ASSERT(stack);
    return stack->Push(error);
  }

  inline void PushError(const std::string& message) {
    ASSERT(!message.empty());
    return PushError(Error::New(message));
  }

  auto VisitPointers(PointerVisitor* vis) -> bool;
  auto VisitPointerPointers(PointerPointerVisitor* vis) -> bool;

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

  template <RuntimeTarget Target>
  inline auto CallPop(Target& target, const ObjectList& args = {}) -> Object* {
    Call(target, args);
    return PopOr();
  }

  inline auto CallPop(Fn& target, const ObjectList& args = {}) -> Object* {
    Call(target, args);
    return PopOr();
  }

  void AddShutdownCallback(const ShutdownCallback& rhs) {
    return shutdown_listeners_.push_back(rhs);
  }

  template <RuntimeTarget Target>
  void AddShutdownListener(Target& rhs) {
    return AddShutdownCallback([this, &rhs]() {
      CallPop(rhs);
    });
  }

  inline void AddShutdownListener(Fn& target) {
    if (target.IsNative())
      return AddShutdownListener(dynamic_cast<NativeFn&>(target));
    else if (target.IsScript())
      return AddShutdownListener(reinterpret_cast<Script&>(target));
    else if (target.IsLambda())
      return AddShutdownListener(reinterpret_cast<Lambda&>(target));
    LOG(FATAL) << "cannot add shutdown listener: " << target;
  }

  void Shutdown();

 private:
  static auto CreateInitScope() -> LocalScope*;

  static inline auto New(LocalScope* init_scope = CreateInitScope()) -> Runtime* {
    return new Runtime(init_scope);
  }

 public:
  static auto Eval(const std::string& expr) -> Object*;
  static auto Exec(Script* script) -> Object*;

 public:
  static void Init(const bool load_kernel = true);
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
