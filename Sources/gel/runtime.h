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
#include "gel/pointer.h"
#include "gel/stack_frame.h"
#include "gel/type_traits.h"

namespace gel {
DECLARE_bool(log_script_instrs);

class ShutdownListener {
  friend class Runtime;
  DEFINE_NON_COPYABLE_TYPE(ShutdownListener);

 public:
  using Callback = std::function<void()>;

  class Iterator {
    DEFINE_NON_COPYABLE_TYPE(Iterator);

   private:
    ShutdownListener* current_;

   public:
    explicit Iterator(ShutdownListener* head) :
      current_(head) {}
    ~Iterator() = default;

    auto HasNext() const -> bool {
      return current_ != nullptr;
    }

    auto Next() -> ShutdownListener* {
      const auto next = current_;
      current_ = current_->GetNext();
      return next;
    }
  };

 private:
  ShutdownListener* next_ = nullptr;
  Callback callback_;

 protected:
  void SetNext(ShutdownListener* rhs) {
    ASSERT(rhs);
    next_ = rhs;
  }

  void OnShutdown() {
    return callback_();
  }

 public:
  ShutdownListener(Callback callback) :
    callback_(callback) {}
  ~ShutdownListener() = default;

  auto GetNext() const -> ShutdownListener* {
    return next_;
  }

  inline auto HasNext() const -> bool {
    return GetNext() != nullptr;
  }

  auto Last() -> ShutdownListener* {
    ShutdownListener* current = this;
    while (current->HasNext()) current = current->GetNext();
    return current;
  }

 public:
  static inline auto New(const Callback& rhs) -> ShutdownListener* {
    return new ShutdownListener(rhs);
  }

  static auto New(Procedure* rhs) -> ShutdownListener*;

  static inline void Append(ShutdownListener** list, ShutdownListener* listener) {
    auto current = (*list);
    if (!current) {
      (*list) = listener;
      return;
    }
    current = current->Last();
    current->SetNext(listener);
  }
};

class Module;
class Runtime {
  friend class CallScope;
  friend class CallStackFrame;
  friend class Collector;
  friend class proc::import;
  friend class proc::exit;
  friend class proc::gel_format;  // TODO: remove
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
  friend class NativeProcedure;
  friend class RuntimeScopeScope;
  friend class StackFrameIterator;
  friend class NativeProcedureEntry;
  DEFINE_NON_COPYABLE_TYPE(Runtime);

 private:
  LocalScope* init_scope_;
  LocalScope* curr_scope_;
  std::stack<StackFrame*> stack_{};
  bool executing_ = false;
  Object* result_ = nullptr;
  ShutdownListener* shutdown_listeners_ = nullptr;
  uint64_t num_shutdown_listeners_ = 0;
  bool emptying_task_queue_ = false;

  inline void SetExecuting(const bool value = true) {
    executing_ = value;
  }

  inline auto GetOperationStack() -> OperationStack* {
    ASSERT(!stack_.empty());
    return stack_.top()->GetOperationStack();
  }

  template <class E>
  inline void CallWithNArgs(E* exec, const uword num_args, std::enable_if_t<gel::is_executable<E>::value>* = nullptr) {
    ASSERT(exec);
    ASSERT(num_args >= 0);
    const auto stack = GetOperationStack();
    ASSERT(stack);
    std::vector<Object*> args{};
    word remaining = static_cast<word>(num_args);
    if (exec->HasArgs()) {
      for (auto idx = 0; idx < exec->GetNumberOfArgs(); idx++) {
        const auto arg = exec->GetArgAt(idx);
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
      args.push_back(Null());
      remaining++;
    }
    return Call(exec, args);
  }

  void Call(NativeProcedure* native, const ObjectList& args = {});
  void Call(Lambda* lambda, const ObjectList& args = {});
  void Call(Script* script, const ObjectList& args = {});
  void Call(Constructor* constructor, const ObjectList& args);

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

  auto PopStackFrame() -> StackFrame*;

  template <class T>
  auto PushStackFrame(T* target, LocalScope* locals, std::enable_if_t<gel::is_stack_frame_target<T>::value>* = nullptr)
      -> const StackFrame* {
    ASSERT(target);
    ASSERT(locals);
    const auto frame_id = HasStackFrame() ? GetCurrentStackFrame()->GetId() + 1 : 1;
    const auto new_frame = new StackFrame(frame_id, target, locals);
    stack_.push(new_frame);
    DVLOG(1000) << "pushed: " << stack_.top()->ToString();
    return stack_.top();
  }

 public:  // TODO: reduce visibility
  void LoadKernel();
  inline void Call(Procedure* procedure, const ObjectList& args = {}) {
    if (procedure->IsLambda()) {
      return Call(procedure->AsLambda(), args);
    } else if (procedure->IsNativeProcedure()) {
      return Call(procedure->AsNativeProcedure(), args);
    }
    LOG(FATAL) << "invalid Call to " << procedure << " w/ args: " << args.size();  // TODO: fix printing args
  }

  template <class T>
  inline void InvokeConstructor(T* this_value, const ObjectList& args = {}) {
    ASSERT(this_value);
    if (!this_value->HasInit())
      return;
    ObjectList invoke_args = {
        this_value,
    };
    invoke_args.insert(std::end(invoke_args), std::begin(args), std::end(args));
    return Call(this_value->GetInit(), args);
  }

 protected:
  explicit Runtime(LocalScope* init_scope = CreateInitScope());
  auto Import(Module* module) -> bool;
  auto Import(Symbol* symbol, LocalScope* scope) -> bool;
  auto ImportModule(const std::string& name) -> bool;

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

  auto HasStackFrame() const -> bool {
    return !stack_.empty();
  }

  auto GetCurrentStackFrame() const -> StackFrame* {
    return stack_.top();
  }

  template <class E>
  inline auto CallPop(E* exec, const ObjectList& args = {}, std::enable_if_t<gel::is_executable<E>::value>* = nullptr)
      -> Object* {
    ASSERT(exec);
    Call(exec, args);
    if (!stack_.empty())
      return result_ = GetOperationStack()->PopOr(Null());
    return result_ ? result_ : (result_ = Null());
  }

  void AddShutdownListener(ShutdownListener* rhs);

  inline void AddShutdownListener(const ShutdownListener::Callback& rhs) {
    return AddShutdownListener(ShutdownListener::New(rhs));
  }

  inline void AddShutdownListener(Procedure* rhs) {
    ASSERT(rhs);
    return AddShutdownListener(ShutdownListener::New(rhs));
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
