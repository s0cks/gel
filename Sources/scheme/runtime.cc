#include "scheme/runtime.h"

#include <glog/logging.h>
#include <units.h>

#include <fstream>
#include <iostream>

#include "scheme/common.h"
#include "scheme/error.h"
#include "scheme/expression.h"
#include "scheme/expression_compiler.h"
#include "scheme/instruction.h"
#include "scheme/interpreter.h"
#include "scheme/local_scope.h"
#include "scheme/module_compiler.h"
#include "scheme/module_resolver.h"
#include "scheme/natives.h"
#include "scheme/os_thread.h"
#include "scheme/parser.h"
#include "scheme/procedure.h"
#include "scheme/tracing.h"
#include "scheme/type.h"

namespace scm {
DEFINE_bool(kernel, true, "Load the kernel at boot.");
DEFINE_string(module_dir, "", "The directories to load modules from.");

static const ThreadLocal<Runtime> runtime_;

auto GetRuntime() -> Runtime* {
  ASSERT(runtime_);
  return runtime_.Get();
}

Runtime::Runtime(LocalScope* scope) :
  ExecutionStack() {
  ASSERT(scope);
  SetScope(scope);
  if (FLAGS_kernel)
    LoadKernelModule();
}

Runtime::~Runtime() {
  if (HasScope()) {
    delete scope_;
    scope_ = nullptr;
  }
}

void Runtime::LoadKernelModule() {
  ASSERT(FLAGS_kernel);
  DVLOG(10) << "loading kernel module....";
  LOG_IF(FATAL, !ImportModule("_kernel")) << "failed to import kernel module.";
}

static inline auto FileExists(const std::string& filename) -> bool {
  std::ifstream file(filename);
  return file.good();
}

class RuntimeModuleResolver : public ModuleResolver {
  DEFINE_NON_COPYABLE_TYPE(RuntimeModuleResolver);

 private:
  RuntimeModuleResolver() = default;

 public:
  ~RuntimeModuleResolver() override = default;

  auto ResolveModule(Symbol* symbol) -> Module* override {
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
    std::stringstream contents;
    contents << file.rdbuf();
    const auto code = contents.str();

    const auto module_expr = Parser::ParseModule(code);
    ASSERT(module_expr);
    const auto module = ModuleCompiler::Compile(module_expr);
    ASSERT(module);
    return module;
  }

 public:
  static inline auto Resolve(Symbol* symbol) -> Module* {
    ASSERT(symbol);
    RuntimeModuleResolver resolver;
    return resolver.ResolveModule(symbol);
  }
};

auto Runtime::ImportModule(Module* module) -> bool {
  ASSERT(module);
  const auto scope = module->GetScope();
  ASSERT(scope);
  scope_->Add(scope);
  modules_.push_back(module);
  return true;
}

auto Runtime::ImportModule(Symbol* symbol) -> bool {
  ASSERT(symbol);
  if (FLAGS_module_dir.empty()) {
    LOG(ERROR) << "cannot import module " << symbol << ", no module dir specified.";
    return true;
  }
  const auto module = RuntimeModuleResolver::Resolve(symbol);
  ASSERT(module);
  return ImportModule(module);
}

template <class Proc>
static inline void RegisterProc(LocalScope* scope, Proc* proc = new Proc()) {
  ASSERT(scope);
  ASSERT(proc);
  const auto symbol = proc->GetSymbol();
  ASSERT(symbol);
  LOG_IF(FATAL, !scope->Add(symbol, proc)) << "failed to register: " << proc->ToString();
  DVLOG(10) << proc->ToString() << " registered.";
}

auto Runtime::CreateInitScope() -> LocalScope* {
  const auto scope = LocalScope::New();
  ASSERT(scope);
  RegisterProc<proc::print>(scope);
  RegisterProc<proc::type>(scope);
  RegisterProc<proc::import>(scope);
  RegisterProc<proc::exit>(scope);
  return scope;
}

auto Runtime::DefineSymbol(Symbol* symbol, Type* value) -> bool {
  ASSERT(symbol);
  ASSERT(value);
  ASSERT(HasScope());
  return GetScope()->Add(symbol, value);
}

auto Runtime::LookupSymbol(Symbol* symbol, Type** result) -> bool {
  ASSERT(symbol);
  LocalVariable* local = nullptr;
  if (!GetScope()->Lookup(symbol, &local))
    return false;
  ASSERT(local);
  (*result) = local->GetValue();
  return local->HasValue();
}

auto Runtime::StoreSymbol(Symbol* symbol, Type* value) -> bool {
  ASSERT(symbol);
  ASSERT(value);
  return DefineSymbol(symbol, value);
}

auto Runtime::Execute(GraphEntryInstr* entry) -> Type* {
  ASSERT(entry && entry->IsGraphEntryInstr());
  PushScope();
  Interpreter interpreter(this);
  interpreter.Run(entry);
  const auto result = Pop();
  PopScope();
  return result;
}

auto Runtime::Eval(GraphEntryInstr* graph_entry) -> Type* {
  ASSERT(HasRuntime());
  ASSERT(graph_entry);
  return GetRuntime()->Execute(graph_entry);
}

auto Runtime::Eval(const std::string& expr) -> Type* {
  ASSERT(!expr.empty());
#ifdef SCM_DEBUG
  LOG(INFO) << "evaluating expression:" << std::endl << expr;
#endif  // SCM_DEBUG
  const auto e = ExpressionCompiler::Compile(expr);
  ASSERT(e && e->HasEntry());
  return Eval(e->GetEntry());
}

void Runtime::Init() {
#ifdef SCM_DEBUG
  const auto start_ts = Clock::now();
#endif  // SCM_DEBUG

  DVLOG(10) << "initializing runtime....";
  Type::Init();
  runtime_.Set(new Runtime());

#ifdef SCM_DEBUG
  const auto stop_ts = Clock::now();
  const auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>((stop_ts - start_ts)).count();
  LOG(INFO) << "runtime initialized in " << units::time::millisecond_t(static_cast<double>(total_ms));
#endif  // SCM_DEBUG
}
}  // namespace scm