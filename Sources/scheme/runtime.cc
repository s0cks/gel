#include "scheme/runtime.h"

#include <glog/logging.h>

#include <iostream>

#include "scheme/common.h"
#include "scheme/expression.h"
#include "scheme/instruction.h"
#include "scheme/tracing.h"
#include "scheme/type.h"

namespace scm {
class print : public Procedure {
  DEFINE_NON_COPYABLE_TYPE(print);

 public:
  print() = default;
  ~print() override = default;

  auto Apply(Runtime* state) const -> bool override {
    ASSERT(state);
    const auto value = state->Pop();
    ASSERT(value);
    PrintValue(std::cout, (*value)) << std::endl;
    return Null::Get();
  }

  auto ToString() const -> std::string override {
    return "print";
  }
};

auto Runtime::CreateInitScope() -> LocalScope* {
  const auto scope = LocalScope::New();
  ASSERT(scope);
  LOG_IF(FATAL, !scope->Add("print", new print())) << "failed to register print procedure.";
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