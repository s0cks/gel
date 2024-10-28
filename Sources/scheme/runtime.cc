#include "scheme/runtime.h"

#include <glog/logging.h>

#include <fstream>
#include <iostream>

#include "scheme/common.h"
#include "scheme/expression.h"
#include "scheme/instruction.h"
#include "scheme/local_scope.h"
#include "scheme/module_compiler.h"
#include "scheme/module_resolver.h"
#include "scheme/natives.h"
#include "scheme/parser.h"
#include "scheme/tracing.h"
#include "scheme/type.h"

namespace scm {
DEFINE_string(module_dir, "", "The directories to load modules from.");

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
    const auto module_filename = fmt::format("{0:s}/{1:s}.ss", FLAGS_module_dir, symbol->Get());
    if (!FileExists(module_filename)) {
      LOG(FATAL) << "cannot load module " << symbol << " from: " << module_filename;
      return nullptr;
    }
    DLOG(INFO) << "importing module " << symbol << " from: " << module_filename;

    std::ifstream file(module_filename, std::ios::in | std::ios::binary);
    ASSERT(file.good());
    std::stringstream contents;
    contents << file.rdbuf();

    const auto code = contents.str();
    DLOG(INFO) << "code:" << std::endl << code;

    const auto module_expr = Parser::ParseModule(code);
    ASSERT(module_expr);
    const auto module = ModuleCompiler::Compile(module_expr);
    ASSERT(module);
    DLOG(INFO) << symbol << " := " << module;
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
  ASSERT(module->IsNamed(symbol));
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
  if (!GetScope()->Lookup(symbol, &local)) {
    LOG(ERROR) << "failed to find local: " << symbol;
    return false;
  }
  ASSERT(local);
  (*result) = local->GetValue();
  return local->HasValue();
}

auto Runtime::LoadSymbol(Symbol* symbol) -> bool {
  ASSERT(symbol);
  Type* result = nullptr;
  if (!LookupSymbol(symbol, &result)) {
    LOG(FATAL) << "failed to find symbol: " << symbol;
    return false;
  }
  ASSERT(result);
  Push(result);
  return true;
}

void Runtime::StoreSymbol(Symbol* symbol, Type* value) {
  ASSERT(symbol);
  ASSERT(value);
  if (!DefineSymbol(symbol, value)) {
    LOG(ERROR) << "failed to set " << symbol << " to: " << value;
    return;
  }
}

static inline auto Add(Type* lhs, Type* rhs) -> Type* {
  ASSERT(lhs && lhs->IsDatum());
  ASSERT(rhs && rhs->IsDatum());
  return lhs->AsDatum()->Add(rhs->AsDatum());
}

static inline auto Subtract(Type* lhs, Type* rhs) -> Type* {
  ASSERT(lhs && lhs->IsDatum());
  ASSERT(rhs && rhs->IsDatum());
  return lhs->AsDatum()->Sub(rhs->AsDatum());
}

static inline auto Multiply(Type* lhs, Type* rhs) -> Type* {
  ASSERT(lhs && lhs->IsDatum());
  ASSERT(rhs && rhs->IsDatum());
  return lhs->AsDatum()->Mul(rhs->AsDatum());
}

static inline auto Divide(Type* lhs, Type* rhs) -> Type* {
  ASSERT(lhs && lhs->IsDatum());
  ASSERT(rhs && rhs->IsDatum());
  return lhs->AsDatum()->Div(rhs->AsDatum());
}

static inline auto Equals(Type* lhs, Type* rhs) -> Datum* {
  ASSERT(lhs);
  ASSERT(rhs);
  return lhs->Equals(rhs) ? Bool::True() : Bool::False();
}

static inline auto Modulus(Type* lhs, Type* rhs) -> Datum* {
  ASSERT(lhs && lhs->IsDatum());
  ASSERT(rhs && rhs->IsDatum());
  return lhs->AsDatum()->Mod(rhs->AsDatum());
}

static inline auto LookupProcedure(LocalScope* scope, Symbol* name, Procedure** result) -> bool {
  ASSERT(scope);
  ASSERT(name);
  LocalVariable* local = nullptr;
  if (!scope->Lookup(name, &local)) {
    (*result) = nullptr;
    LOG(ERROR) << "failed to resolve procedure named: " << name->Get();
    return false;
  }
  ASSERT(local);
  if (!local->HasValue()) {
    LOG(ERROR) << "failed to get value for: " << (*local);
    (*result) = nullptr;
    return false;
  }
  const auto value = local->GetValue();
  if (!value->IsProcedure()) {
    (*result) = nullptr;
    LOG(ERROR) << "'" << name->Get() << "' (" << value->ToString() << ") is not a procedure.";
    return false;
  }
  (*result) = value->AsProcedure();
  return true;
}

auto Runtime::VisitGraphEntryInstr(GraphEntryInstr* instr) -> bool {
  return true;
}

auto Runtime::VisitTargetEntryInstr(TargetEntryInstr* instr) -> bool {
  return true;
}

auto Runtime::VisitJoinEntryInstr(JoinEntryInstr* instr) -> bool {
  return true;
}

auto Runtime::CallProcedure(Procedure* procedure) -> bool {
  ASSERT(procedure);
  return procedure->Apply(this);
}

auto Runtime::VisitCallProcInstr(CallProcInstr* instr) -> bool {
  ASSERT(instr);
  const auto symbol = instr->GetSymbol();
  ASSERT(symbol);
  Procedure* proc = nullptr;
  if (!LookupProcedure(GetScope(), symbol, &proc))
    return false;
  ASSERT(proc);
  return CallProcedure(proc);
}

auto Runtime::VisitConstantInstr(ConstantInstr* instr) -> bool {
  const auto value = instr->GetValue();
  ASSERT(value);
  Push(value);
  return true;
}

auto Runtime::VisitStoreVariableInstr(StoreVariableInstr* instr) -> bool {
  const auto value = Pop();
  ASSERT(value);
  const auto symbol = instr->GetSymbol();
  ASSERT(symbol);
  StoreSymbol(symbol, *value);
  return true;
}

auto Runtime::VisitLoadVariableInstr(LoadVariableInstr* instr) -> bool {
  return LoadSymbol(instr->GetSymbol());
}

auto Runtime::VisitReturnInstr(ReturnInstr* instr) -> bool {
  return true;
}

auto Runtime::VisitGotoInstr(GotoInstr* instr) -> bool {
  ASSERT(instr);
  ASSERT(instr->HasTarget());
  SetCurrentInstr(instr->GetTarget());
  return true;
}

static inline auto Truth(scm::Type* rhs) -> bool {
  ASSERT(rhs);
  if (rhs->IsBool())
    return rhs->AsBool()->Get();
  return !rhs->IsNull();  // TODO: better truth?
}

auto Runtime::VisitBranchInstr(BranchInstr* instr) -> bool {
  ASSERT(instr);
  const auto test = Pop();
  if (!test) {
    DLOG(ERROR) << "no test value to pop from stack.";
    return false;
  }
  const auto target = Truth(*test) ? instr->GetTrueTarget() : instr->GetFalseTarget();
  ASSERT(target);
  SetCurrentInstr(target);
  return true;
}

auto Runtime::VisitBinaryOpInstr(BinaryOpInstr* instr) -> bool {
  const auto op = instr->GetOp();
  const auto right = Pop();
  ASSERT(right);
  const auto left = Pop();
  ASSERT(left);
  switch (op) {
    case expr::kAdd:
      Push(Add(*left, *right));
      return true;
    case expr::kSubtract:
      Push(Subtract(*left, *right));
      return true;
    case expr::kMultiply:
      Push(Multiply(*left, *right));
      return true;
    case expr::kDivide:
      Push(Divide(*left, *right));
      return true;
    case expr::kEquals:
      Push(Equals(*left, *right));
      return true;
    case expr::kModulus:
      Push(Modulus(*left, *right));
      return true;
    default:
      LOG(ERROR) << "invalid BinaryOp: " << op;
      return false;
  }
}

void Runtime::ExecuteInstr(Instruction* instr) {
  ASSERT(instr);
  TRACE_SECTION(ExecuteInstr);
  TRACE_TAG(instr->GetName());
  DVLOG(10) << "executing: " << instr->GetName();
  if (!instr->Accept(this))
    LOG(FATAL) << "failed to execute: " << instr->ToString();
}

auto Runtime::Execute(EntryInstr* entry) -> Type* {
  TRACE_BEGIN;
  ASSERT(entry);
  SetCurrentInstr(entry->GetFirstInstruction());
  while (HasCurrentInstr()) {
    const auto next = GetCurrentInstr();
    ASSERT(next);
    ExecuteInstr(next);
    if (next->IsEntryInstr()) {
      current_ = (dynamic_cast<EntryInstr*>(next))->GetFirstInstruction();
    } else if (!next->IsBranchInstr() && !next->IsGotoInstr()) {
      current_ = next->GetNext();
    }
  }

  const auto result = Pop();
  return result.value_or(Null::Get());
}

auto Runtime::EvalWithScope(FlowGraph* flow_graph, LocalScope* scope) -> Type* {
  ASSERT(flow_graph);
  ASSERT(scope);
  Runtime runtime(scope);
  return runtime.Execute(flow_graph->GetEntry());
}
}  // namespace scm