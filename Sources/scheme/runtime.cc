#include "scheme/runtime.h"

#include <glog/logging.h>
#include <units.h>

#include <fstream>
#include <iostream>
#include <ranges>

#include "scheme/common.h"
#include "scheme/error.h"
#include "scheme/expression.h"
#include "scheme/expression_compiler.h"
#include "scheme/instruction.h"
#include "scheme/interpreter.h"
#include "scheme/local.h"
#include "scheme/local_scope.h"
#include "scheme/native_procedure.h"
#include "scheme/natives.h"
#include "scheme/object.h"
#include "scheme/os_thread.h"
#include "scheme/parser.h"
#include "scheme/procedure.h"
#include "scheme/tracing.h"

namespace scm {
DEFINE_bool(kernel, true, "Load the kernel at boot.");
DEFINE_string(module_dir, "", "The directories to load modules from.");

static const ThreadLocal<Runtime> runtime_;

auto GetRuntime() -> Runtime* {
  ASSERT(runtime_);
  return runtime_.Get();
}

Runtime::Runtime(LocalScope* scope) :
  ExecutionStack(),
  scope_(scope),
  interpreter_(this) {
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

auto Runtime::Apply(Procedure* proc, const std::vector<Object*>& args) -> Object* {
  const auto stack_size = GetStackSize();
  if (proc->IsProcedure()) {
    for (const auto& arg : args) {
      Push(arg);
    }
    proc->Apply();
  } else if (proc->IsNativeProcedure()) {
    if (!proc->AsNativeProcedure()->Apply(args))
      return Error::New("cannot invoke procedure");
  }
  const auto result = GetStackSize() > stack_size ? Pop() : nullptr;
  return result;
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

template <class Proc>
static inline void RegisterProc(LocalScope* scope, Proc* proc = new Proc()) {
  ASSERT(scope);
  ASSERT(proc);
  const auto symbol = proc->GetSymbol();
  ASSERT(symbol);
  LOG_IF(FATAL, !scope->Add(symbol, proc)) << "failed to register: " << proc->ToString();
}

auto Runtime::CreateInitScope() -> LocalScope* {
  const auto scope = LocalScope::New();
  ASSERT(scope);
  RegisterProc<proc::print>(scope);
  RegisterProc<proc::type>(scope);
  RegisterProc<proc::import>(scope);
  RegisterProc<proc::exit>(scope);
  RegisterProc<proc::format>(scope);
  RegisterProc<proc::list>(scope);
#ifdef SCM_DEBUG
  RegisterProc<proc::frame>(scope);
  scope->Add("debug?", Bool::True());
  RegisterProc<proc::list_symbols>(scope);
  RegisterProc<proc::list_classes>(scope);
#endif  // SCM_DEBUG
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
  const auto frame = GetCurrentFrame();
  ASSERT(frame);
  const auto locals = frame->GetLocals();
  ASSERT(locals);
  LocalVariable* local = nullptr;
  if (!locals->Lookup(symbol, &local))
    return false;
  ASSERT(local);
  (*result) = local->GetValue();
  return local->HasValue();
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

void Runtime::Call(instr::TargetEntryInstr* target, LocalScope* locals) {
  ASSERT(target && target->HasNext());
  const auto frame = interpreter_.PushStackFrame(locals);
  ASSERT(frame);
  interpreter_.Execute(target, locals);
}

void Runtime::Call(Lambda* lambda) {
  ASSERT(lambda);
  const auto locals = LocalScope::New(GetCurrentScope());
  ASSERT(locals);
  for (const auto& arg : std::ranges::reverse_view(lambda->GetArgs())) {
    const auto symbol = Symbol::New(arg.GetName());
    ASSERT(symbol);
    const auto value = Pop();
    ASSERT(value);
    const auto local = LocalVariable::New(locals, symbol, value);
    ASSERT(local);
    if (!locals->Add(local))
      throw Exception("failed to add parameter local");
  }
  return Call(lambda->GetEntry()->GetTarget(), locals);
}

void Runtime::Call(NativeProcedure* native, const std::vector<Object*>& args) {
  ASSERT(native);
  const auto locals = LocalScope::New(GetCurrentScope());
  ASSERT(locals);
  for (auto idx = 0; idx < args.size(); idx++) {
    locals->Add(Symbol::New(fmt::format("arg{}", idx)), args[idx]);
  }
  const auto frame = interpreter_.PushStackFrame(locals);
  ASSERT(frame);
  if (!native->Apply(args))
    throw Exception(fmt::format("failed to apply procedure: {}", native->ToString()));
  interpreter_.PopStackFrame();
}

auto Runtime::Eval(const std::string& expr) -> Object* {
  ASSERT(!expr.empty());
  DVLOG(10) << "evaluating expression:" << std::endl << expr;
  const auto runtime = GetRuntime();
  ASSERT(runtime);
  const auto scope = runtime->GetGlobalScope();
  const auto e = ExpressionCompiler::Compile(expr, scope);
  ASSERT(e && e->HasEntry());
  ASSERT(!runtime->HasFrame());
  runtime->Call(e->GetEntry()->GetTarget(), scope);
  ASSERT(!runtime->HasFrame());
  return runtime->Pop();
}

auto Runtime::Exec(Script* script) -> Object* {
  ASSERT(script && script->IsCompiled());
  const auto runtime = GetRuntime();
  ASSERT(runtime);
  const auto scope = LocalScope::Union(
      {
          script->GetScope(),
      },
      runtime->GetGlobalScope());
  ASSERT(!runtime->HasFrame());
  runtime->Call(script->GetEntry()->GetTarget(), scope);
  ASSERT(!runtime->HasFrame());
  return runtime->Pop();
}

void Runtime::Init() {
#ifdef SCM_DEBUG
  const auto start_ts = Clock::now();
#endif  // SCM_DEBUG

  DVLOG(10) << "initializing runtime....";
  Object::Init();
  const auto runtime = new Runtime();
  runtime_.Set(runtime);
  if (FLAGS_kernel)
    runtime->LoadKernelModule();

#ifdef SCM_DEBUG
  const auto stop_ts = Clock::now();
  const auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>((stop_ts - start_ts)).count();
  LOG(INFO) << "runtime initialized in " << units::time::millisecond_t(static_cast<double>(total_ms));
#endif  // SCM_DEBUG
}
}  // namespace scm