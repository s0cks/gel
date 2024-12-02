#include "gel/runtime.h"

#include <glog/logging.h>
#include <units.h>

#include <fstream>
#include <iostream>
#include <ranges>

#include "gel/common.h"
#include "gel/error.h"
#include "gel/expression.h"
#include "gel/expression_compiler.h"
#include "gel/instruction.h"
#include "gel/interpreter.h"
#include "gel/lambda.h"
#include "gel/local.h"
#include "gel/local_scope.h"
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
DEFINE_string(module_dir, "", "The directories to load modules from.");

static const ThreadLocal<Runtime> runtime_;

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

void Runtime::LoadKernelModule() {
  ASSERT(FLAGS_kernel);
  DVLOG(10) << "loading kernel module....";
  LOG_IF(FATAL, !Import("_kernel", GetGlobalScope())) << "failed to import kernel module.";
}

static inline auto FileExists(const std::string& filename) -> bool {
  std::ifstream file(filename);
  return file.good();
}

class ScriptResolver {
  DEFINE_NON_COPYABLE_TYPE(ScriptResolver);

 protected:
  ScriptResolver() = default;

 public:
  virtual ~ScriptResolver() = default;
  virtual auto ResolveScript(Symbol* symbol) -> Script* = 0;
};
class RuntimeScriptResolver : public ScriptResolver {
  DEFINE_NON_COPYABLE_TYPE(RuntimeScriptResolver);

 private:
  LocalScope* scope_;

  explicit RuntimeScriptResolver(LocalScope* scope) :
    scope_(scope) {
    ASSERT(scope_);
  }

 public:
  ~RuntimeScriptResolver() override = default;

  auto GetScope() const -> LocalScope* {
    return scope_;
  }

  auto ResolveScript(Symbol* symbol) -> Script* override {
    ASSERT(symbol);
    ASSERT(!FLAGS_module_dir.empty());
    const auto module_filename = fmt::format("{0:s}/{1:s}.cl", FLAGS_module_dir, symbol->Get());
    if (!FileExists(module_filename)) {
      LOG(FATAL) << "cannot load module " << symbol << " from: " << module_filename;
      return nullptr;
    }
    DVLOG(10) << "importing module " << symbol << " from: " << module_filename;

    std::ifstream file(module_filename, std::ios::in | std::ios::binary);
    ASSERT(file.good());
    return Parser::ParseScript(file, GetScope());
  }

 public:
  static inline auto Resolve(Symbol* symbol, LocalScope* scope) -> Script* {
    ASSERT(symbol);
    RuntimeScriptResolver resolver(scope);
    return resolver.ResolveScript(symbol);
  }
};

auto Runtime::Import(Script* script) -> bool {
  ASSERT(script);
  const auto scope = script->GetScope();
  ASSERT(scope);
  scope_->Add(scope);
  scripts_.push_back(script);
  return true;
}

auto Runtime::Import(Symbol* symbol, LocalScope* scope) -> bool {
  ASSERT(symbol);
  if (FLAGS_module_dir.empty()) {
    LOG(ERROR) << "cannot import module " << symbol << ", no module dir specified.";
    return true;
  }
  const auto module = RuntimeScriptResolver::Resolve(symbol, scope);
  ASSERT(module);
  return Import(module);
}

auto Runtime::CreateInitScope() -> LocalScope* {
  const auto scope = LocalScope::New();
  ASSERT(scope);
  RegisterNative<proc::print>(scope);
  RegisterNative<proc::type>(scope);
  RegisterNative<proc::import>(scope);
  RegisterNative<proc::exit>(scope);
  RegisterNative<proc::format>(scope);
  RegisterNative<proc::list>(scope);
  RegisterNative<proc::set_car>(scope);
  RegisterNative<proc::set_cdr>(scope);
  RegisterNative<proc::random>(scope);
  RegisterNative<proc::rand_range>(scope);
  RegisterNative<proc::array_new>(scope);
  RegisterNative<proc::array_get>(scope);
  RegisterNative<proc::array_set>(scope);
  RegisterNative<proc::array_length>(scope);

#ifdef GEL_ENABLE_RX
#define REGISTER_RX(Name)                                                              \
  ({                                                                                   \
    const auto [symbol, procedure] = RegisterNative<proc::rx_##Name>(scope);           \
    const auto local = LocalVariable::New(rx_scope, symbol, procedure);                \
    ASSERT(local);                                                                     \
    LOG_IF(FATAL, !rx_scope->Add(local)) << "failed to add rx scope value: " << local; \
  })

  {
    const auto rx_scope = rx::GetRxScope();
    REGISTER_RX(observer);
    REGISTER_RX(observable);
    REGISTER_RX(subscribe);
    REGISTER_RX(first);
    REGISTER_RX(last);
    REGISTER_RX(map);
    REGISTER_RX(take);
    REGISTER_RX(take_last);
    REGISTER_RX(skip);
    REGISTER_RX(buffer);
    REGISTER_RX(filter);
    REGISTER_RX(take_while);
    REGISTER_RX(replay_subject);
    REGISTER_RX(publish_subject);
    REGISTER_RX(publish);
    REGISTER_RX(complete);
    REGISTER_RX(publish_error);
  }

#undef REGISTER_RX
#endif  // GEL_ENABLE_RX

#ifdef GEL_DEBUG
  RegisterNative<proc::gel_minor_gc>(scope);
  RegisterNative<proc::gel_major_gc>(scope);
  RegisterNative<proc::gel_get_frame>(scope);
  RegisterNative<proc::gel_get_debug>(scope);
  RegisterNative<proc::gel_get_target_triple>(scope);
  RegisterNative<proc::gel_get_locals>(scope);
  RegisterNative<proc::gel_get_classes>(scope);
#endif  // GEL_DEBUG
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
  ASSERT(e && e->HasEntry());
  const auto init_frame = runtime->GetCurrentFrame();
  // TODO: runtime->Call(e->GetEntry()->GetTarget(), scope);
  const auto post_frame = runtime->GetCurrentFrame();
  ASSERT(runtime->HasError() || (!init_frame && !post_frame) || (*init_frame) == (*post_frame));
  return runtime->Pop();
}

auto Runtime::Exec(Script* script) -> Object* {
  ASSERT(script && script->IsCompiled());
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
  if (FLAGS_kernel)
    runtime->LoadKernelModule();

#ifdef GEL_DEBUG
  const auto stop_ts = Clock::now();
  const auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>((stop_ts - start_ts)).count();
  LOG(INFO) << "runtime initialized in " << units::time::millisecond_t(static_cast<double>(total_ms));
#endif  // GEL_DEBUG
}
}  // namespace gel