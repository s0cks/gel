#include "gel/runtime.h"

#include <glog/logging.h>
#include <units.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <ranges>
#include <unordered_set>

#include "gel/common.h"
#include "gel/error.h"
#include "gel/expr/expression.h"
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

namespace fs = std::filesystem;

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

auto Runtime::CreateInitScope() -> LocalScope* {
  const auto scope = LocalScope::New();
  ASSERT(scope);
  return scope;
}

void Runtime::Call(Lambda* lambda, const ObjectList& args) {
  ASSERT(lambda);
  const auto locals = PushScope();
  ASSERT(locals);
  const auto self_local = LocalVariable::New(locals, lambda->HasSymbol() ? lambda->GetSymbol() : Symbol::New("this"), lambda);
  ASSERT(self_local);
  LOG_IF(FATAL, !locals->Add(self_local)) << "failed to add " << (*self_local) << " to scope.";
  {
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
    if (!lambda->IsCompiled())
      LOG_IF(FATAL, !FlowGraphCompiler::Compile(lambda, locals)) << "failed to compile: " << lambda;
    {
      StackFrameGuard<Lambda> stack_guard(lambda);
      PushStackFrame(lambda, locals);
      Interpreter interpreter(this);
      interpreter.Run(lambda->GetCode().GetStartingAddress());
      const auto frame = PopStackFrame();
      ASSERT(frame);
      const auto result = frame->HasReturnAddress() ? frame->GetReturnObjectPointer() : Null();
      ASSERT(result);
      if (HasStackFrame()) {
        GetCurrentStackFrame()->GetOperationStack()->Push(result);
      } else {
        result_ = result;
      }
    }
  }
  PopScope();
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

void Runtime::Call(NativeProcedure* native, const ObjectList& args) {
  ASSERT(native && native->HasEntry());
  const auto locals = PushScope();
  ASSERT(locals);
  {
    for (auto idx = 0; idx < args.size(); idx++) {
      locals->Add(Symbol::New(fmt::format("arg{}", idx)), args[idx]);
    }
    StackFrameGuard<NativeProcedure> guard(native);
    {
      PushStackFrame(native, locals);
      LOG_IF(FATAL, !native->GetEntry()->Apply(args)) << "failed to apply: " << native->ToString() << " with args: " << args;
      const auto frame = PopStackFrame();
      ASSERT(frame);
      const auto result = frame->HasReturnAddress() ? frame->GetReturnObjectPointer() : Null();
      ASSERT(result);
      if (HasStackFrame()) {
        GetCurrentStackFrame()->GetOperationStack()->Push(result);
      } else {
        result_ = result;
      }
    }
  }
  PopScope();
}

void Runtime::Call(Script* script, const ObjectList& args) {
  ASSERT(script && script->IsCompiled());
  const auto locals = PushScope();
  ASSERT(locals);
  {
    locals->AddAll(script->GetScope());
    StackFrameGuard<Script> stack_guard(script);
    {
      PushStackFrame(script, locals);
      Interpreter interpreter(this);
      interpreter.Run(script->GetCode().GetStartingAddress());
      const auto frame = PopStackFrame();
      ASSERT(frame);
      const auto result = frame->HasReturnAddress() ? frame->GetReturnObjectPointer() : Null();
      ASSERT(result);
      if (HasStackFrame()) {
        GetCurrentStackFrame()->GetOperationStack()->Push(result);
      } else {
        result_ = result;
      }
    }
  }
  PopScope();
}

auto Runtime::Eval(const std::string& expr) -> Object* {
  ASSERT(!expr.empty());
  DVLOG(10) << "evaluating expression:" << std::endl << expr;
  const auto runtime = GetRuntime();
  ASSERT(runtime);
  const auto scope = runtime->PushScope();
  const auto args = Array<Argument*>::New(0);
  ASSERT(args);
  const auto lambda = Lambda::New(args, {});
  ASSERT(lambda);
  const auto this_local = LocalVariable::New(scope, "this", lambda);
  ASSERT(this_local);
  LOG_IF(FATAL, !scope->Add(this_local)) << "failed to add " << (*this_local) << " to scope.";
  const auto parsed = Parser::ParseExpr(expr, scope);
  if (parsed)
    lambda->SetBody(parsed);
  LOG_IF(FATAL, !FlowGraphCompiler::Compile(lambda, scope)) << "failed to compile: " << expr;
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

auto Runtime::PushStackFrame(NativeProcedure* native, LocalScope* locals) -> const StackFrame* {
  ASSERT(locals);
  const auto frame_id = HasStackFrame() ? GetCurrentStackFrame()->GetId() + 1 : 1;
  const auto new_frame = new StackFrame(frame_id, native, locals);
  stack_.push(new_frame);
  DVLOG(1000) << "pushed: " << stack_.top()->ToString();
  return stack_.top();
}

auto Runtime::PushStackFrame(Script* target, LocalScope* locals) -> const StackFrame* {
  ASSERT(target);
  const auto frame_id = HasStackFrame() ? GetCurrentStackFrame()->GetId() + 1 : 1;
  const auto new_frame = new StackFrame(frame_id, target, locals);
  stack_.push(new_frame);
  DVLOG(1000) << "pushed: " << stack_.top()->ToString();
  return stack_.top();
}

auto Runtime::PushStackFrame(Lambda* target, LocalScope* locals) -> const StackFrame* {
  ASSERT(target);
  const auto frame_id = HasStackFrame() ? GetCurrentStackFrame()->GetId() + 1 : 1;
  const auto new_frame = new StackFrame(frame_id, target, locals);
  stack_.push(new_frame);
  DVLOG(1000) << "pushed: " << stack_.top()->ToString();
  return stack_.top();
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
}  // namespace gel