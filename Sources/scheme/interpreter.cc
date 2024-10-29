#include "scheme/interpreter.h"

#include "scheme/error.h"
#include "scheme/instruction.h"
#include "scheme/runtime.h"

namespace scm {
auto Interpreter::VisitLoadVariableInstr(LoadVariableInstr* instr) -> bool {
  return GetRuntime()->LoadSymbol(instr->GetSymbol());
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
  const auto error = Error::New(GetRuntime()->Pop());
  throw error;
  return true;
}

auto Interpreter::VisitInvokeInstr(InvokeInstr* instr) -> bool {
  ASSERT(instr);
  const auto target = GetRuntime()->Pop();
  if (!IsProcedure(target))
    throw Exception(fmt::format("expected {0:s} to be a Procedure.", target ? target->ToString() : "null"));
  const auto procedure = target->AsProcedure();
  ASSERT(procedure);
  return procedure->Apply(GetRuntime());
}

auto Interpreter::VisitCallProcInstr(CallProcInstr* instr) -> bool {
  ASSERT(instr);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto Interpreter::VisitInvokeNativeInstr(InvokeNativeInstr* instr) -> bool {
  ASSERT(instr);
  const auto target = GetRuntime()->Pop();
  if (!IsNativeProcedure(target))
    throw Exception(fmt::format("expected {0:s} to be a NativeProcedure.", target ? target->ToString() : "null"));
  const auto procedure = target->AsProcedure();
  ASSERT(procedure);
  return procedure->Apply(GetRuntime());
}

auto Interpreter::VisitBranchInstr(BranchInstr* instr) -> bool {
  ASSERT(instr);
  const auto test = GetRuntime()->Pop();
  if (!test) {
    DLOG(ERROR) << "no test value to pop from stack.";
    return false;
  }
  const auto target = Truth(test) ? instr->GetTrueTarget() : instr->GetFalseTarget();
  ASSERT(target);
  SetCurrentInstr(target);
  return true;
}

static inline auto ApplyBinaryOp(BinaryOp op, Type* lhs, Type* rhs) -> Type* {
  switch (op) {
    case expr::kAdd:
      return Add(lhs, rhs);
    case expr::kSubtract:
      return Subtract(lhs, rhs);
    case expr::kMultiply:
      return Multiply(lhs, rhs);
    case expr::kDivide:
      return Divide(lhs, rhs);
    case expr::kEquals:
      return Equals(lhs, rhs);
    case expr::kModulus:
      return Modulus(lhs, rhs);
    case expr::kBinaryAnd:
      return BinaryAnd(lhs, rhs);
    case expr::kBinaryOr:
      return BinaryOr(lhs, rhs);
    default:
      LOG(FATAL) << "invalid BinaryOp: " << op;
      return nullptr;
  }
}

auto Interpreter::VisitBinaryOpInstr(BinaryOpInstr* instr) -> bool {
  const auto right = GetRuntime()->Pop();
  ASSERT(right);
  const auto left = GetRuntime()->Pop();
  ASSERT(left);
  const auto result = ApplyBinaryOp(instr->GetOp(), left, right);
  ASSERT(result);
  GetRuntime()->Push(result);
  return true;
}

auto Interpreter::VisitTargetEntryInstr(TargetEntryInstr* instr) -> bool {
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
  return GetRuntime()->StoreSymbol(symbol, value);
}

auto Interpreter::VisitGraphEntryInstr(GraphEntryInstr* instr) -> bool {
  return true;
}

void Interpreter::ExecuteInstr(Instruction* instr) {
  ASSERT(instr);
  DVLOG(10) << "executing " << instr->ToString();
  LOG_IF(FATAL, !instr->Accept(this)) << "failed to execute: " << instr->ToString();
  if (instr->IsBranchInstr() || instr->IsGotoInstr())
    return;
  SetCurrentInstr(instr->GetNext());
}

void Interpreter::Run(GraphEntryInstr* entry) {
  ASSERT(entry && entry->HasNext());
  SetCurrentInstr(entry->GetFirstInstruction());
  while (HasCurrentInstr()) {
    const auto next = GetCurrentInstr();
    ASSERT(next);
    ExecuteInstr(next);
  }
}
}  // namespace scm