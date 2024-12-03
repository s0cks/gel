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
#include "gel/expression.h"
#include "gel/expression_compiler.h"
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
#include "gel/procedure.h"
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
  ExecutionStack(),
  init_scope_(scope),
  scope_(scope),
  interpreter_(this) {
  ASSERT(init_scope_);
  ASSERT(scope_);
}

Runtime::~Runtime() {
  if (scope_) {
    delete scope_;
    scope_ = nullptr;
  }
}

namespace fs = std::filesystem;

void Runtime::LoadKernelModule() {
  if (!FLAGS_kernel)
    return;
  const auto home = kHomeVar.value();
  if (!home) {
    LOG(WARNING) << "${GEL_HOME} environment variable not set, skipping loading kernel.";
    return;
  }
  const auto kernel = Module::LoadFrom(fmt::format("{}/_kernel.cl", (*home)));
  LOG_IF(FATAL, !kernel) << "failed to load the _kernel Module.";
  LOG_IF(FATAL, !GetGlobalScope()->Add(kernel->GetScope())) << "failed to import the _kernel Module.";
  std::unordered_set<std::string> paths;
  paths.insert(fmt::format("{}/lib", (*home)));
  if (kPathVar) {
    Split(*(kPathVar.value()), ';', paths);
  }
  for (const auto& path : paths) {
    DVLOG(10) << "loading Modules from " << path << "....";
    DirModuleLoader loader(path);
    LOG_IF(ERROR, !loader.LoadAllModules()) << "failed to load Modules from " << path;
  }
}

static inline auto FileExists(const std::string& filename) -> bool {  // TODO: remove this
  std::ifstream file(filename);
  return file.good();
}

auto Runtime::Import(Module* m) -> bool {
  ASSERT(m);
  return scope_->Add(m->GetScope());
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

auto Runtime::DefineSymbol(Symbol* symbol, Object* value) -> bool {
  ASSERT(symbol);
  ASSERT(value);
  const auto frame = GetCurrentFrame();
  ASSERT(frame);
  const auto locals = frame->GetLocals();
  ASSERT(locals);
  return locals->Add(symbol, value);
}

auto Runtime::LookupSymbol(Symbol* symbol, Object** result) -> bool {
  ASSERT(symbol);
  const auto scope = GetCurrentScope();
  ASSERT(scope);
  LocalVariable* local = nullptr;
  if (!scope->Lookup(symbol, &local))
    return false;
  ASSERT(local);
  (*result) = local->GetValue();
  return true;
}

auto Runtime::StoreSymbol(Symbol* symbol, Object* value) -> bool {
  ASSERT(symbol);
  ASSERT(value);
  const auto frame = GetCurrentFrame();
  ASSERT(frame);
  const auto locals = frame->GetLocals();
  ASSERT(locals);
  LocalVariable* local = nullptr;
  if (!locals->Lookup(symbol, &local))
    return locals->Add(symbol, value);
  local->SetValue(value);
  return true;
}

void Runtime::Call(Lambda* lambda, const ObjectList& args) {
  ASSERT(lambda);
  const auto locals = LocalScope::New(GetCurrentScope());
  ASSERT(locals);
  if (!lambda->IsCompiled()) {
    if (!LambdaCompiler::Compile(lambda, locals)) {
      LOG(FATAL) << "failed to compile: " << lambda;
      return;
    }
  }
  const auto& lambda_args = lambda->GetArgs();
  ASSERT(lambda_args.size() == args.size());
  auto idx = 0;
  for (const auto& arg : std::ranges::reverse_view(lambda_args)) {
    const auto symbol = Symbol::New(arg.GetName());
    ASSERT(symbol);
    const auto value = args[idx++];
    ASSERT(value);
    const auto local = LocalVariable::New(locals, symbol, value);
    ASSERT(local);
    LOG_IF(FATAL, !locals->Add(local)) << "failed to add parameter local";
  }
  StackFrameGuard<Lambda> stack_guard(lambda);
  interpreter_.Execute(lambda, locals);
}

void Runtime::Call(NativeProcedure* native, const ObjectList& args) {
  ASSERT(native);
  const auto locals = LocalScope::New(GetCurrentScope());
  ASSERT(locals);
  for (auto idx = 0; idx < args.size(); idx++) {
    locals->Add(Symbol::New(fmt::format("arg{}", idx)), args[idx]);
  }
  StackFrameGuard<NativeProcedure> guard(native);
  interpreter_.PushStackFrame(native, locals);
  LOG_IF(FATAL, !native->Apply(args)) << "failed to apply: " << native->ToString() << " with args: " << args;
  interpreter_.PopStackFrame();
}

void Runtime::Call(Script* script) {
  ASSERT(script && script->IsCompiled());
  StackFrameGuard<Script> stack_guard(script);
  interpreter_.Execute(script, LocalScope::Union({script->GetScope()}, GetInitScope()));
}

auto Runtime::Eval(const std::string& expr) -> Object* {
  ASSERT(!expr.empty());
  DVLOG(10) << "evaluating expression:" << std::endl << expr;
  const auto runtime = GetRuntime();
  ASSERT(runtime);
  const auto scope = runtime->GetGlobalScope();
  const auto e = ExpressionCompiler::Compile(expr, scope);
  ASSERT(e);
  return nullptr;  // TODO: implement
}

auto Runtime::Exec(Script* script) -> Object* {
  ASSERT(script);
  if (!script->HasEntry())
    return Null();
  const auto runtime = GetRuntime();
  ASSERT(runtime);
  if (FLAGS_log_script_instrs) {
    LOG(INFO) << "Script instructions:";
    InstructionLogger::Log(script->GetEntry());
  }
  return GetRuntime()->CallPop(script);
}

void Runtime::Init() {
#ifdef GEL_DEBUG
  const auto start_ts = Clock::now();
#endif  // GEL_DEBUG

  DVLOG(10) << "initializing runtime....";
  Object::Init();
  const auto runtime = new Runtime();
  runtime_.Set(runtime);
  runtime->LoadKernelModule();

#ifdef GEL_DEBUG
  const auto stop_ts = Clock::now();
  const auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>((stop_ts - start_ts)).count();
  LOG(INFO) << "runtime initialized in " << units::time::millisecond_t(static_cast<double>(total_ms));
#endif  // GEL_DEBUG
}
}  // namespace gel