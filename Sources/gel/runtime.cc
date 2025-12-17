#include "gel/runtime.h"

#include <filesystem>
#include <fstream>
#include <glog/logging.h>
#include <iostream>
#include <ranges>
#include <type_traits>
#include <units.h>
#include <unordered_set>

#include "gel/call_stack.h"
#include "gel/common.h"
#include "gel/type/error.h"
#include "gel/event_loop.h"
#include "gel/exception.h"
#include "gel/frontend/expr/expr.h"
#include "gel/frontend/flow_graph_compiler.h"
#include "gel/instruction.h"
#include "gel/vm/interpreter.h"
#include "gel/local.h"
#include "gel/local_scope.h"
#include "gel/type/module.h"
#include "gel/module_loader.h"
#include "gel/type/object.h"
#include "gel/os_thread.h"
#include "gel/frontend/parser.h"
#include "gel/heap/pointer.h"
#include "gel/profiling.h"
#include "gel/script.h"
#include "gel/vm/stack_frame.h"
#include "gel/thread_local.h"
#include "gel/tracing.h"

namespace gel {
DEFINE_bool(kernel, true, "Load the kernel at boot.");
DEFINE_bool(log_script_instrs, false, "Log the Script instructions before execution");

static const ThreadLocal<Runtime> runtime_;
static const EnvironmentVariable kHomeVar("GEL_HOME");
static const EnvironmentVariable kPathVar("GEL_PATH");

auto GetHomeEnvVar() -> const EnvironmentVariable& {
  return kHomeVar;
}

auto GetRuntime() -> Runtime* {
  ASSERT(runtime_);
  return runtime_.Get();
}

Runtime::Runtime(LocalScope* scope) :
  init_scope_(scope),
  curr_scope_(scope) {
  ASSERT(init_scope_);
  ASSERT(curr_scope_);
}

class ModuleImporter {
  DEFINE_NON_COPYABLE_TYPE(ModuleImporter);

 protected:
  ModuleImporter() = default;

 public:
  virtual ~ModuleImporter() = default;
  virtual void Import(Module* m) = 0;
};

class BaseModuleImporter : public ModuleImporter {
  DEFINE_NON_COPYABLE_TYPE(BaseModuleImporter);

 private:
  Runtime* runtime_;

 protected:
  explicit BaseModuleImporter(Runtime* runtime) :
    ModuleImporter(),
    runtime_(runtime) {
    ASSERT(runtime_);
  }

  virtual void ImportScope(LocalScope* scope) = 0;

 public:
  ~BaseModuleImporter() override = default;

  auto GetRuntime() const -> Runtime* {
    return runtime_;
  }

  void Import(Module* m) override {
    ASSERT(m);
    return ImportScope(m->GetScope());
  }
};

class KernelModuleImporter : public BaseModuleImporter {
  DEFINE_NON_COPYABLE_TYPE(KernelModuleImporter);

 protected:
  void ImportScope(LocalScope* scope) override {
    ASSERT(scope);
    if (VLOG_IS_ON(100)) {
      DLOG(INFO) << "importing " << scope->ToString() << ":";
      LocalScopePrinter::Print<google::INFO, false>(scope, __FILE__, __LINE__);
    }
    GetRuntime()->GetInitScope()->AddAll(scope);
  }

 public:
  explicit KernelModuleImporter(Runtime* runtime) :
    BaseModuleImporter(runtime) {}
  ~KernelModuleImporter() override = default;

  void Import(Module* m) override {
    ASSERT(m && m->IsKernel());
    BaseModuleImporter::Import(m);
    // import default namespace
    const auto default_ns = m->GetDefaultNamespace();
    ASSERT(default_ns);
    ImportScope(default_ns->GetScope());
    if (!default_ns->GetSymbol()->Equals("gel")) {
      const auto gel_ns = m->FindNamespace("gel");
      if (gel_ns) {
        ImportScope(gel_ns->GetScope());
        return;
      }
      DLOG(WARNING) << "cannot find `gel namespace in: " << m->ToString();
    }
  }
};

class DefaultModuleImporter : public BaseModuleImporter {
  DEFINE_NON_COPYABLE_TYPE(DefaultModuleImporter);

 protected:
  void ImportScope(LocalScope* scope) override {
    ASSERT(scope);
    if (VLOG_IS_ON(100)) {
      DLOG(INFO) << "importing " << scope->ToString() << ":";
      LocalScopePrinter::Print<google::INFO, false>(scope, __FILE__, __LINE__);
    }
    GetRuntime()->GetScope()->AddAll(scope);
  }

 public:
  explicit DefaultModuleImporter(Runtime* runtime) :
    BaseModuleImporter(runtime) {}
  ~DefaultModuleImporter() override = default;

  void Import(Module* m) override {
    ASSERT(m);
    BaseModuleImporter::Import(m);
    // import default namespace
    const auto default_ns = m->GetDefaultNamespace();
    ASSERT(default_ns);
    ImportScope(default_ns->GetScope());
    if (!default_ns->GetSymbol()->Equals("gel")) {
      const auto gel_ns = m->FindNamespace("gel");
      if (gel_ns) {
        ImportScope(gel_ns->GetScope());
        return;
      }
      DLOG(WARNING) << "cannot find `gel namespace in: " << m->ToString();
    }
  }
};

void Runtime::LoadKernel() {
  const auto loader = GetThreadKernelModuleLoader();
  ASSERT(loader);
  KernelModuleImporter importer(this);
  const auto kernel = loader->LoadModule("gel.cl");
  LOG_IF(FATAL, !kernel) << "failed to load kernel module: " << kernel;
  ASSERT(kernel && kernel->IsKernel());
  importer.Import(kernel);

#ifdef GEL_DEBUG
  const auto kernel_dbg = loader->LoadModule("geldbg.cl");
  LOG_IF(FATAL, !kernel_dbg) << "failed to load kernel debug module: " << kernel_dbg;
  ASSERT(kernel_dbg && kernel_dbg->IsKernel());
  importer.Import(kernel_dbg);
#endif  // GEL_DEBUG
}

auto GetGelPathEnvVar() -> const EnvironmentVariable& {
  static EnvironmentVariable kVar("GEL_PATH");
  return kVar;
}

static inline auto FileExists(const std::string& filename) -> bool {  // TODO: remove this
  std::ifstream file(filename);
  return file.good();
}

auto Runtime::Import(Module* m) -> bool {
  ASSERT(m);
  curr_scope_->AddAll(m->GetScope());
  return true;
}

auto Runtime::Import(Symbol* symbol, LocalScope* scope) -> bool {
  ASSERT(symbol);
  const auto home = kHomeVar.value();
  LOG_IF(FATAL, !home) << "no $" << kHomeVar.name() << " variable set in environment.";
  auto module = nullptr;
  return Import(module);
}

auto Runtime::ImportModule(const std::string& name) -> bool {
  const auto loader = GetThreadModuleLoader();
  ASSERT(loader);
  ASSERT(!name.empty());
  DefaultModuleImporter importer(this);
  const auto m = loader->LoadModule(name);
  LOG_IF(FATAL, !m) << "failed to import module: " << m;
  importer.Import(m);
  return true;
}

auto Runtime::CreateInitScope() -> LocalScope* {
  const auto scope = LocalScope::New();
  ASSERT(scope);
  return scope;
}

class CallScope {
  DEFINE_NON_COPYABLE_TYPE(CallScope);

 private:
  Runtime* runtime_;
  LocalScope* scope_{};

 public:
  CallScope(Runtime* runtime) :
    runtime_(runtime) {
    ASSERT(runtime_);
    scope_ = runtime_->PushScope();
  }
  ~CallScope() {
    ASSERT(runtime_);
    runtime_->PopScope();
  }

  auto operator->() const -> LocalScope* {
    ASSERT(scope_);
    return scope_;
  }

  operator LocalScope*() const {
    ASSERT(scope_);
    return scope_;
  }
};

class CallStackFrame {
  DEFINE_NON_COPYABLE_TYPE(CallStackFrame);

 private:
  CallStack& stack_;

  inline auto stack() const -> CallStack& {
    return stack_;
  }

 public:
  template <StackFrameTarget Target>
  CallStackFrame(CallStack& call_stack, Target* target, LocalScope* locals) :
    stack_(call_stack) {
    ASSERT(target);
    ASSERT(locals);
    stack().PushStackFrame(target, locals);
  }
  ~CallStackFrame() {
    const auto frame = stack().Pop();
    ASSERT(frame);
    const auto result = frame->HasReturnAddress() ? frame->GetReturnObjectPointer() : Nil::Get();
    ASSERT(result);
    if (!stack().IsEmpty()) {
      stack().GetTop()->GetOperationStack()->Push(result);
    } else {
      GetRuntime()->result_ = result;
    }
  }
};

template <>
void Runtime::Call(InitFn& init, const ObjectList args) {
  {
    CallScope locals(this);
    if (init.HasScope())
      locals->AddAll(init.GetScope());
    if (init.HasArgs()) {
      const auto& lambda_args = init.GetArgs();
      ASSERT(lambda_args);
      for (int idx = static_cast<int>(args.size()); idx > 0; idx--) {
        const auto arg = lambda_args->Get(static_cast<uword>(idx) - 1);
        ASSERT(arg);
        ASSERT((idx - 1) == arg->GetIndex());
        const auto symbol = Symbol::New(arg->GetName()->Get());
        ASSERT(symbol);
        const auto value = args[args.size() - idx];
        ASSERT(value);
        const auto local = LocalVariable::New(locals, symbol, value);
        ASSERT(local);
        LOG_IF(FATAL, !locals->Add(local)) << "failed to add parameter: " << (*local);
      }
    }
    DLOG(INFO) << init << " scope:";
    PRINT_SCOPE(INFO, locals);
    LOG_IF(FATAL, !FlowGraphCompiler::Compile(init, locals)) << "failed to compile: " << init;
    {
      StackFrameGuard<InitFn> stack_guard(&init);
      CallStackFrame call_frame(GetCallStack(), &init, locals);
      Interpreter interpreter(this);
      interpreter(init);
    }
  }

  if (ShouldEmptyTaskQueue())
    EmptyTaskQueue();
}

template <>
void Runtime::Call(LambdaFn& lambda, const ObjectList args) {
  {
    CallScope locals(this);
    if (lambda.HasScope())
      locals->AddAll(lambda.GetScope());
    if (lambda.HasArgs()) {
      const auto& lambda_args = lambda.GetArgs();
      ASSERT(lambda_args);
      for (int idx = static_cast<int>(args.size()); idx > 0; idx--) {
        const auto arg = lambda_args->Get(static_cast<uword>(idx) - 1);
        ASSERT(arg);
        ASSERT((idx - 1) == arg->GetIndex());
        const auto symbol = Symbol::New(arg->GetName()->Get());
        ASSERT(symbol);
        const auto value = args[args.size() - idx];
        ASSERT(value);
        const auto local = LocalVariable::New(locals, symbol, value);
        ASSERT(local);
        LOG_IF(FATAL, !locals->Add(local)) << "failed to add parameter: " << (*local);
      }
    }
    DLOG(INFO) << lambda << " scope:";
    PRINT_SCOPE(INFO, locals);
    LOG_IF(FATAL, !FlowGraphCompiler::Compile(lambda, locals)) << "failed to compile: " << lambda;
    {
      StackFrameGuard<LambdaFn> stack_guard(&lambda);
      CallStackFrame call_frame(GetCallStack(), &lambda, locals);
      Interpreter interpreter(this);
      interpreter(lambda);
    }
  }

  if (ShouldEmptyTaskQueue())
    EmptyTaskQueue();
}

template <>
void Runtime::Call(NativeFn& native, const ObjectList args) {
  if (!native.IsLinked())
    throw IllegalArgumentException(fmt::format("`{}` is not linked", native));
  {
    CallScope locals(this);
    for (auto idx = 0; idx < args.size(); idx++) {
      locals->Add(Symbol::New(fmt::format("arg{}", idx)), args[idx]);
    }
    {
      StackFrameGuard<NativeFn> guard(&native);
      CallStackFrame stack_frame(GetCallStack(), &native, locals);
      LOG_IF(FATAL, !native(args)) << "failed to apply `" << native << "` with args: " << args;
    }
  }

  if (ShouldEmptyTaskQueue())
    EmptyTaskQueue();
}

template <>
void Runtime::Call(Script& script, const ObjectList args) {
  {
    CallScope locals(this);
    if (script.HasScope())
      locals->AddAll(script.GetScope());
    {
      StackFrameGuard<Script> stack_guard(&script);
      CallStackFrame stack_frame(GetCallStack(), &script, locals);
      Interpreter interpreter(this);
      interpreter(script);
    }
  }

  if (ShouldEmptyTaskQueue())
    EmptyTaskQueue();
}

void Runtime::Call(Fn& target, const ObjectList args) {
  if (target.IsInitFn())
    return Call(reinterpret_cast<InitFn&>(target), std::move(args));
  else if (target.IsNative())
    return Call(dynamic_cast<NativeFn&>(target), std::move(args));
  else if (target.IsScript())
    return Call(reinterpret_cast<Script&>(target), std::move(args));
  else if (target.IsLambdaFn())
    return Call(reinterpret_cast<LambdaFn&>(target), std::move(args));
  LOG(FATAL) << "invalid call to " << target << " with args: " << args;
}

void Runtime::EmptyTaskQueue() {
  const auto loop = GetThreadEventLoop();
  ASSERT(loop);
  auto& tasks = loop->GetTaskQueue();
  emptying_task_queue_ = true;
  while (!tasks.empty()) {
    auto next = tasks.front();
    tasks.pop_front();
    next.Execute();
  }
  emptying_task_queue_ = false;
  loop->Run(UV_RUN_NOWAIT);
}

auto Runtime::VisitPointers(PointerVisitor* vis) -> bool {
  ASSERT(vis);
  return vis->Visit(curr_scope_->raw_ptr());
}

auto Runtime::VisitPointerPointers(PointerPointerVisitor* vis) -> bool {
  ASSERT(vis);
  auto current_scope = curr_scope_->raw_ptr();
  if (!vis->Visit(&current_scope))
    return false;
  if (!curr_scope_->raw_ptr()->Equals(current_scope)) {
    curr_scope_ = current_scope->As<LocalScope>();
    ASSERT(curr_scope_);
  }
  return true;
}

auto Runtime::Eval(const std::string& expr) -> Object* {
  ASSERT(!expr.empty());
  const auto runtime = GetRuntime();
  const auto lambda = Parser::ParseExpr(expr);
  ASSERT(lambda);
  LOG_IF(FATAL, !FlowGraphCompiler::Compile(*lambda, runtime->GetInitScope())) << "failed to compile: " << expr;
  const auto result = runtime->CallPop(*lambda);
  runtime->PopScope();
  return result ? result : Nil::Get();
}

auto Runtime::Exec(Script* script) -> Object* {
  ASSERT(script);
  const auto runtime = GetRuntime();
  ASSERT(runtime);
  return GetRuntime()->CallPop(*script);
}

void Runtime::Init(const bool load_kernel) {
  GEL_PROFILE;
  const auto runtime = new Runtime();
  runtime_.Set(runtime);
  Object::Init();
  if (load_kernel && FLAGS_kernel) {
    KernelModuleLoader::Init();
    runtime->LoadKernel();
  }
  ThreadModuleLoader::Init();
}

void Runtime::Shutdown() {
  invoke_all(shutdown_listeners_);
}

}  // namespace gel
