#include "gel/runtime.h"

#include <filesystem>
#include <fstream>
#include <glog/logging.h>
#include <iostream>
#include <ranges>
#include <type_traits>
#include <units.h>
#include <unordered_set>

#include "gel/common.h"
#include "gel/error.h"
#include "gel/event_loop.h"
#include "gel/expression.h"
#include "gel/flow_graph_compiler.h"
#include "gel/instruction.h"
#include "gel/interpreter.h"
#include "gel/lambda.h"
#include "gel/local.h"
#include "gel/local_scope.h"
#include "gel/module.h"
#include "gel/module_loader.h"
#include "gel/native_procedure.h"
#include "gel/natives.h"
#include "gel/object.h"
#include "gel/os_thread.h"
#include "gel/parser.h"
#include "gel/pointer.h"
#include "gel/procedure.h"
#include "gel/script.h"
#include "gel/stack_frame.h"
#include "gel/thread_local.h"
#include "gel/tracing.h"

namespace gel {
DEFINE_bool(kernel, true, "Load the kernel at boot.");
DEFINE_bool(log_script_instrs, false, "Log the Script instructions before execution");

static const ThreadLocal<Runtime> runtime_;
static const EnvironmentVariable kHomeVar("GEL_HOME");
static const EnvironmentVariable kPathVar("GEL_PATH");

auto ShutdownListener::New(Procedure* rhs) -> ShutdownListener* {
  ASSERT(rhs);
  return New([rhs]() {
    GetRuntime()->Call(rhs);
  });
}

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
    if (default_ns->GetName() != "gel") {
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
    if (default_ns->GetName() != "gel") {
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

 public:
  template <class T>
  CallStackFrame(T* target, LocalScope* locals, std::enable_if_t<gel::is_stack_frame_target<T>::value>* = nullptr) {
    ASSERT(runtime_);
    ASSERT(target);
    ASSERT(locals);
    GetRuntime()->PushStackFrame(target, locals);
  }
  ~CallStackFrame() {
    const auto frame = GetRuntime()->PopStackFrame();
    ASSERT(frame);
    const auto result = frame->HasReturnAddress() ? frame->GetReturnObjectPointer() : Null();
    ASSERT(result);
    if (GetRuntime()->HasStackFrame()) {
      GetRuntime()->GetCurrentStackFrame()->GetOperationStack()->Push(result);
    } else {
      GetRuntime()->result_ = result;
    }
  }
};

void Runtime::Call(Constructor* init, const ObjectList& args) {
  ASSERT(init);
  {
    CallScope locals(this);
    if (init->HasScope())
      locals->AddAll(init->GetScope());
    if (init->HasArgs()) {
      const auto& lambda_args = init->GetArgs();
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
    LOG_IF(FATAL, !FlowGraphCompiler::Compile(init, locals)) << "failed to compile: " << init;
    {
#ifdef GEL_DEBUG

      if (VLOG_IS_ON(10)) {
        DLOG(INFO) << init->ToString() << " execution scope:";
        PRINT_SCOPE(INFO, locals);
      }

#endif  // GEL_DEBUG
      StackFrameGuard<Constructor> stack_guard(init);
      CallStackFrame call_frame(init, locals);
      Interpreter interpreter(this);
      interpreter.Run<Constructor>(init);
    }
  }

  if (!HasStackFrame() && !emptying_task_queue_) {
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
}

void Runtime::Call(Lambda* lambda, const ObjectList& args) {
  ASSERT(lambda);
  {
    CallScope locals(this);
    if (lambda->HasScope())
      locals->AddAll(lambda->GetScope());
    if (lambda->HasArgs()) {
      const auto& lambda_args = lambda->GetArgs();
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
    LOG_IF(FATAL, !FlowGraphCompiler::Compile(lambda, locals)) << "failed to compile: " << lambda;
    {
#ifdef GEL_DEBUG

      if (VLOG_IS_ON(10)) {
        DLOG(INFO) << lambda->ToString() << " execution scope:";
        PRINT_SCOPE(INFO, locals);
      }

#endif  // GEL_DEBUG
      StackFrameGuard<Lambda> stack_guard(lambda);
      CallStackFrame call_frame(lambda, locals);
      Interpreter interpreter(this);
      interpreter.Run(lambda);
    }
  }

  if (!HasStackFrame() && !emptying_task_queue_) {
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
}

void Runtime::Call(NativeProcedure* native, const ObjectList& args) {
  ASSERT(native);
  if (!native->HasEntry()) {
    std::stringstream ss;
    ss << "NativeProcedure `" << native->GetSymbol()->GetFullyQualifiedName() << "` is not linked.";
    throw Exception(ss.str());
    return;
  }
  {
    CallScope locals(this);
    for (auto idx = 0; idx < args.size(); idx++) {
      locals->Add(Symbol::New(fmt::format("arg{}", idx)), args[idx]);
    }
    {
#ifdef GEL_DEBUG

      if (VLOG_IS_ON(10)) {
        DLOG(INFO) << native->ToString() << " execution scope:";
        PRINT_SCOPE(INFO, locals);
      }

#endif  // GEL_DEBUG
      StackFrameGuard<NativeProcedure> guard(native);
      CallStackFrame stack_frame(native, locals);
      LOG_IF(FATAL, !native->GetEntry()->Apply(args))
          << "failed to apply: " << native->ToString() << " with args: " << args;
    }
  }

  if (!HasStackFrame() && !emptying_task_queue_) {
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
}

void Runtime::Call(Script* script, const ObjectList& args) {
  ASSERT(script);
  {
    CallScope locals(this);
    if (script->HasScope())
      locals->AddAll(script->GetScope());
    {
#ifdef GEL_DEBUG

      if (VLOG_IS_ON(10)) {
        DLOG(INFO) << script->ToString() << " execution scope:";
        PRINT_SCOPE(INFO, locals);
      }

#endif  // GEL_DEBUG
      StackFrameGuard<Script> stack_guard(script);
      CallStackFrame stack_frame(script, locals);
      Interpreter interpreter(this);
      interpreter.Run(script);
    }
  }

  if (!HasStackFrame() && !emptying_task_queue_) {
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
  DVLOG(10) << "evaluating expression:" << std::endl << expr;
  const auto runtime = GetRuntime();
  const auto lambda = Parser::ParseExpr(expr);
  ASSERT(lambda);
  LOG_IF(FATAL, !FlowGraphCompiler::Compile(lambda, runtime->GetInitScope())) << "failed to compile: " << expr;
  const auto result = runtime->CallPop(lambda);
  runtime->PopScope();
  return result ? result : Null();
}

auto Runtime::Exec(Script* script) -> Object* {
  ASSERT(script);
  const auto runtime = GetRuntime();
  ASSERT(runtime);
  return GetRuntime()->CallPop(script);
}

void Runtime::Init(const bool load_kernel) {
#ifdef GEL_DEBUG
  const auto start_ts = Clock::now();
#endif  // GEL_DEBUG

  const auto runtime = new Runtime();
  runtime_.Set(runtime);
  Object::Init();
  if (load_kernel && FLAGS_kernel) {
    KernelModuleLoader::Init();
    runtime->LoadKernel();
  }
  ThreadModuleLoader::Init();

#ifdef GEL_DEBUG
  const auto stop_ts = Clock::now();
  const auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>((stop_ts - start_ts)).count();
  DVLOG(1) << "runtime initialized in " << units::time::millisecond_t(static_cast<double>(total_ms));
#endif  // GEL_DEBUG
}

auto Runtime::PopStackFrame() -> StackFrame* {
  if (stack_.empty()) {
    DLOG(ERROR) << "stack empty";
    return nullptr;
  }
  ASSERT(!stack_.empty());
  const auto frame = stack_.top();
  stack_.pop();
  DVLOG(1000) << "popped: " << frame->ToString();
  return frame;
}

void Runtime::AddShutdownListener(ShutdownListener* rhs) {
  ASSERT(rhs);
  ShutdownListener::Append(&shutdown_listeners_, rhs);
  num_shutdown_listeners_ += 1;
}

void Runtime::Shutdown() {
  ShutdownListener::Iterator iter(shutdown_listeners_);
  while (iter.HasNext()) {
    const auto next = iter.Next();
    ASSERT(next);
    next->OnShutdown();
  }
}
}  // namespace gel