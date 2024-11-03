#include "scheme/interpreter.h"

#include <algorithm>
#include <ranges>

#include "scheme/common.h"
#include "scheme/error.h"
#include "scheme/expression.h"
#include "scheme/instruction.h"
#include "scheme/native_procedure.h"
#include "scheme/runtime.h"
#include "scheme/type.h"

namespace scm {
auto Interpreter::VisitLoadVariableInstr(LoadVariableInstr* instr) -> bool {
  ASSERT(instr);
  const auto symbol = instr->GetSymbol();
  ASSERT(symbol);
  Type* result = nullptr;
  if (!GetRuntime()->LookupSymbol(symbol, &result)) {
    LOG(ERROR) << "failed to find symbol: " << symbol;
    GetRuntime()->Push(Error::New(fmt::format("failed to find Symbol: `{0:s}`", symbol->Get())));
    return true;
  }

  ASSERT(result);
  GetRuntime()->Push(result);
  return true;
}

auto Interpreter::VisitConsInstr(ConsInstr* instr) -> bool {
  ASSERT(instr);
  const auto cdr = GetRuntime()->Pop();
  ASSERT(cdr);
  const auto car = GetRuntime()->Pop();
  ASSERT(car);
  GetRuntime()->Push(Pair::New(car, cdr));
  return true;
}

auto Interpreter::VisitReturnInstr(ReturnInstr* instr) -> bool {
  GetRuntime()->PopScope();
  return true;
}

static inline auto Unary(const expr::UnaryOp op, Type* rhs) -> Type* {
  ASSERT(rhs);
  switch (op) {
    case expr::kNot:
      return Not(rhs);
    case expr::kCar:
      return Car(rhs);
    case expr::kCdr:
      return Cdr(rhs);
    default:
      LOG(FATAL) << "invalid UnaryOp: " << op;
      return nullptr;
  }
}

auto Interpreter::VisitUnaryOpInstr(UnaryOpInstr* instr) -> bool {
  ASSERT(instr);
  const auto value = GetRuntime()->Pop();
  if (!value) {
    LOG(FATAL) << "invalid value.";
    return false;
  }

  const auto result = Unary(instr->GetOp(), value);
  ASSERT(result);
  GetRuntime()->Push(result);
  return true;
}

auto Interpreter::VisitGotoInstr(GotoInstr* instr) -> bool {
  ASSERT(instr);
  ASSERT(instr->HasTarget());
  SetCurrentInstr(instr->GetTarget());
  return true;
}

auto Interpreter::VisitThrowInstr(ThrowInstr* instr) -> bool {
  ASSERT(instr);
  GetRuntime()->Push(Error::New(GetRuntime()->Pop()));
  return true;
}

auto Interpreter::VisitInvokeInstr(InvokeInstr* instr) -> bool {
  ASSERT(instr);
  const auto target = GetRuntime()->Pop();
  if (!IsProcedure(target))
    throw Exception(fmt::format("expected {0:s} to be a Procedure.", target ? target->ToString() : "null"));
  const auto procedure = target->AsProcedure();
  ASSERT(procedure);
  GetRuntime()->PushScope();
  const auto result = procedure->Apply(GetRuntime());
  GetRuntime()->PopScope();
  return result;
}

auto Interpreter::VisitInvokeDynamicInstr(InvokeDynamicInstr* instr) -> bool {
  ASSERT(instr);
  const auto target = GetRuntime()->Pop();
  ASSERT(target->IsSymbol());
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return true;
}

auto Interpreter::VisitEvalInstr(EvalInstr* instr) -> bool {
  ASSERT(instr);
  const auto value = GetRuntime()->Pop();
  ASSERT(value && value->IsString());
  const auto result = GetRuntime()->Eval(String::Unbox(value));
  ASSERT(result);
  GetRuntime()->Push(result);
  return true;
}

auto Interpreter::VisitInvokeNativeInstr(InvokeNativeInstr* instr) -> bool {
  ASSERT(instr);
  const auto target = GetRuntime()->Pop();
  if (!target || !target->IsNativeProcedure())
    throw Exception(fmt::format("expected {0:s} to be a NativeProcedure.", target ? target->ToString() : "null"));
  const auto procedure = target->AsProcedure();
  ASSERT(procedure && procedure->IsNative());
  std::vector<Type*> args{};
  for (auto idx = 0; idx < instr->GetNumberOfArgs(); idx++) {
    args.push_back(GetRuntime()->Pop());
  }
  std::ranges::reverse(std::begin(args), std::end(args));

  const auto native = procedure->AsNativeProcedure();
  ASSERT(native);
  GetRuntime()->PushScope();
  if (!native->Apply(args))
    throw Exception(fmt::format("failed to apply procedure: {}", native->ToString()));
  GetRuntime()->PopScope();
  return true;
}

static inline auto GetTarget(const bool branch, instr::BranchInstr* instr) -> instr::EntryInstr* {
  if (branch)
    return instr->GetTrueTarget();
  if (instr->HasFalseTarget())
    return instr->GetFalseTarget();
  return instr->GetJoin();
}

auto Interpreter::VisitBranchInstr(BranchInstr* instr) -> bool {
  ASSERT(instr);
  const auto test = GetRuntime()->Pop();
  ASSERT(test);
  const auto target = GetTarget(Truth(test), instr);
  SetCurrentInstr(target ? target : instr->GetNext());
  return true;
}

auto Interpreter::VisitInstanceOfInstr(InstanceOfInstr* instr) -> bool {
  ASSERT(instr);
  const auto stack_top = GetRuntime()->StackTop().value_or(Null::Get());
  ASSERT(stack_top);
  const auto predicate = instr->GetPredicate();
  if (!predicate(stack_top)) {
    GetRuntime()->PushError(fmt::format("unexpected: {}", stack_top->ToString()));
    return true;
  }
  return true;
}

static inline auto ApplyBinaryOp(BinaryOp op, Datum* lhs, Datum* rhs) -> Datum* {
  switch (op) {
    case expr::kAdd:
      return lhs->Add(rhs);
    case expr::kSubtract:
      return lhs->Sub(rhs);
    case expr::kMultiply:
      return lhs->Mul(rhs);
    case expr::kDivide:
      return lhs->Div(rhs);
    case expr::kEquals:
      return Bool::Box(lhs->Equals(rhs));
    case expr::kModulus:
      return lhs->Mod(rhs);
    case expr::kBinaryAnd:
      return lhs->And(rhs);
    case expr::kBinaryOr:
      return lhs->Or(rhs);
    case expr::kGreaterThan:
      return Bool::Box(lhs->Compare(rhs) > 0);
    case expr::kGreaterThanEqual:
      return Bool::Box(lhs->Compare(rhs) >= 0);
    case expr::kLessThan:
      return Bool::Box(lhs->Compare(rhs) < 0);
    case expr::kLessThanEqual:
      return Bool::Box(lhs->Compare(rhs) <= 0);
    default:
      LOG(FATAL) << "invalid BinaryOp: " << op;
      return nullptr;
  }
}

auto Interpreter::VisitBinaryOpInstr(BinaryOpInstr* instr) -> bool {
  const auto right = GetRuntime()->Pop();
  ASSERT(right && right->IsDatum());
  const auto left = GetRuntime()->Pop();
  ASSERT(left && left->IsDatum());
  const auto result = ApplyBinaryOp(instr->GetOp(), left->AsDatum(), right->AsDatum());
  ASSERT(result);
  GetRuntime()->Push(result);
  DVLOG(100) << left << " " << instr->GetOp() << " " << right << " := " << result;
  return true;
}

auto Interpreter::VisitTargetEntryInstr(TargetEntryInstr* instr) -> bool {
  GetRuntime()->PushScope();
  return true;
}

auto Interpreter::VisitJoinEntryInstr(JoinEntryInstr* instr) -> bool {
  return true;
}

auto Interpreter::VisitConstantInstr(ConstantInstr* instr) -> bool {
  const auto value = instr->GetValue();
  ASSERT(value);
  GetRuntime()->Push(value);
  return true;
}

auto Interpreter::VisitStoreVariableInstr(StoreVariableInstr* instr) -> bool {
  const auto value = GetRuntime()->Pop();
  ASSERT(value);
  const auto symbol = instr->GetSymbol();
  ASSERT(symbol);
  if (!GetRuntime()->StoreSymbol(symbol, value)) {
    LOG(ERROR) << "failed to store symbol " << symbol << " to value: " << value;
    return false;
  }
  return true;
}

auto Interpreter::VisitGraphEntryInstr(GraphEntryInstr* instr) -> bool {
  return true;
}

void Interpreter::ExecuteInstr(Instruction* instr) {
  ASSERT(instr);
  DVLOG(100) << "executing " << instr->ToString();
  LOG_IF(FATAL, !instr->Accept(this)) << "failed to execute: " << instr->ToString();
  if (instr->IsBranchInstr() || instr->IsGotoInstr())
    return;
  SetCurrentInstr(instr->GetNext());
}

void Interpreter::Run(GraphEntryInstr* entry) {
  ASSERT(entry && entry->HasNext());
  SetCurrentInstr(entry);
  while (HasCurrentInstr() && !GetRuntime()->HasError()) {
    const auto next = GetCurrentInstr();
    ASSERT(next);
    ExecuteInstr(next);
  }
}
}  // namespace scm