#include "scheme/interpreter.h"

#include <glog/logging.h>

#include <iostream>

#include "scheme/common.h"
#include "scheme/expression.h"
#include "scheme/instruction.h"
#include "scheme/tracing.h"

namespace scm {
class print : public Procedure {
  DEFINE_NON_COPYABLE_TYPE(print);

 public:
  print() = default;
  ~print() override = default;

  auto Apply(Environment* env, Datum* rhs) const -> Datum* override {
    ASSERT(rhs);
    PrintValue(std::cout, rhs) << std::endl;
    return Null::Get();
  }

  auto ToString() const -> std::string override {
    return "print";
  }
};

static inline auto CreateGlobalEnvironment() -> Environment* {
  const auto env = Environment::New();
  ASSERT(env);
  env->Put("print", new print());
  return env;
}

Interpreter::Interpreter() {
  SetState(State::New(CreateGlobalEnvironment()));
}

Interpreter::~Interpreter() {
  delete state_;
}

auto Interpreter::DefineSymbol(Symbol* symbol, Type* value) -> bool {
  ASSERT(HasState());
  ASSERT(symbol);
  ASSERT(value);
  const auto state = GetState();
  ASSERT(state);
  const auto globals = state->GetGlobals();
  ASSERT(globals);
  return globals->Put(symbol->Get(), value);
}

auto Interpreter::LookupSymbol(Symbol* symbol, Type** result) -> bool {
  ASSERT(HasState());
  ASSERT(symbol);
  const auto globals = GetState()->GetGlobals();
  ASSERT(globals);
  return globals->Lookup(symbol->Get(), result);
}

auto Interpreter::LoadSymbol(Symbol* symbol) -> bool {
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

void Interpreter::StoreSymbol(Symbol* symbol, Type* value) {
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

static inline auto LookupProcedure(Environment* env, Symbol* name, Procedure** result) -> bool {
  ASSERT(env);
  ASSERT(name);
  Type* value = nullptr;
  if (!env->Lookup(name, &value)) {
    (*result) = nullptr;
    LOG(ERROR) << "failed to resolve procedure named: " << name->Get();
    return false;
  }
  ASSERT(value);
  if (!value->IsProcedure()) {
    (*result) = nullptr;
    LOG(ERROR) << "'" << name->Get() << "' (" << value->ToString() << ") is not a procedure.";
    return false;
  }
  (*result) = value->AsProcedure();
  return true;
}

auto Interpreter::VisitGraphEntryInstr(GraphEntryInstr* instr) -> bool {
  return true;
}

auto Interpreter::VisitTargetEntryInstr(TargetEntryInstr* instr) -> bool {
  return true;
}

auto Interpreter::VisitJoinEntryInstr(JoinEntryInstr* instr) -> bool {
  return true;
}

auto Interpreter::VisitCallProcInstr(CallProcInstr* instr) -> bool {
  ASSERT(instr);
  const auto symbol = instr->GetSymbol();
  ASSERT(symbol);
  Procedure* proc = nullptr;
  if (!LookupProcedure(GetState()->GetGlobals(), symbol, &proc))
    return false;
  ASSERT(proc);
  ASSERT(proc->IsProcedure());
  const auto args = Pop();  // TODO: fetch args
  ASSERT(args->IsDatum());
  const auto result = proc->Apply(GetState()->GetGlobals(), args->AsDatum());
  ASSERT(result);
  Push(result);
  return true;
}

auto Interpreter::VisitConstantInstr(ConstantInstr* instr) -> bool {
  const auto value = instr->GetValue();
  ASSERT(value);
  Push(value);
  return true;
}

auto Interpreter::VisitStoreVariableInstr(StoreVariableInstr* instr) -> bool {
  const auto value = Pop();
  ASSERT(value);
  ASSERT(value->IsDatum());
  const auto symbol = instr->GetSymbol();
  ASSERT(symbol);
  StoreSymbol(symbol, value);
  return true;
}

auto Interpreter::VisitLoadVariableInstr(LoadVariableInstr* instr) -> bool {
  return LoadSymbol(instr->GetSymbol());
}

auto Interpreter::VisitReturnInstr(ReturnInstr* instr) -> bool {
  ASSERT(!GetState()->IsStackEmpty());
  return true;
}

auto Interpreter::VisitGotoInstr(GotoInstr* instr) -> bool {
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

auto Interpreter::VisitBranchInstr(BranchInstr* instr) -> bool {
  ASSERT(instr);
  if (Truth(Pop())) {
    SetCurrentInstr(instr->GetTrueTarget());
  } else {
    SetCurrentInstr(instr->GetFalseTarget());
  }
  return true;
}

auto Interpreter::VisitBinaryOpInstr(BinaryOpInstr* instr) -> bool {
  const auto op = instr->GetOp();
  const auto right = Pop();
  const auto left = Pop();
  switch (op) {
    case expr::kAdd:
      Push(Add(left, right));
      return true;
    case expr::kSubtract:
      Push(Subtract(left, right));
      return true;
    case expr::kMultiply:
      Push(Multiply(left, right));
      return true;
    case expr::kDivide:
      Push(Divide(left, right));
      return true;
    case expr::kEquals:
      Push(Equals(left, right));
      return true;
    case expr::kModulus:
      Push(Modulus(left, right));
      return true;
    default:
      LOG(ERROR) << "invalid BinaryOp: " << op;
      return false;
  }
}

void Interpreter::ExecuteInstr(Instruction* instr) {
  ASSERT(instr);
  TRACE_SECTION(ExecuteInstr);
  TRACE_TAG(instr->GetName());
  DVLOG(10) << "executing: " << instr->ToString();
  if (!instr->Accept(this))
    LOG(FATAL) << "failed to execute: " << instr->ToString();
}

auto Interpreter::Execute(EntryInstr* entry) -> Type* {
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

  const auto state = GetState();
  ASSERT(state);
  if (state->IsStackEmpty())
    return Null::Get();

  const auto result = Pop();
  ASSERT(result);
  ASSERT(state->IsStackEmpty());
  return result;
}
}  // namespace scm