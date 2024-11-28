#include "scheme/interpreter.h"

#include <algorithm>
#include <ranges>

#include "scheme/common.h"
#include "scheme/error.h"
#include "scheme/expression.h"
#include "scheme/instruction.h"
#include "scheme/lambda.h"
#include "scheme/local_scope.h"
#include "scheme/native_procedure.h"
#include "scheme/object.h"
#include "scheme/platform.h"
#include "scheme/runtime.h"

namespace scm {
auto Interpreter::GetStackTop() const -> std::optional<Stack::value_type> {
  return GetRuntime()->StackTop();
}

auto Interpreter::PushError(const std::string& message) -> bool {
  ASSERT(!message.empty());
  GetRuntime()->PushError(message);
  return true;
}

auto Interpreter::PushNext(Object* rhs) -> bool {
  ASSERT(rhs);
  GetRuntime()->Push(rhs);
  return Next();
}

auto Interpreter::VisitLoadVariableInstr(LoadVariableInstr* instr) -> bool {
  ASSERT(instr);
  const auto symbol = instr->GetSymbol();
  ASSERT(symbol);
  Object* result = nullptr;
  if (!GetRuntime()->LookupSymbol(symbol, &result)) {
    DLOG(ERROR) << "failed to find " << symbol << " in scope: ";
    PRINT_SCOPE(ERROR, GetRuntime()->GetCurrentScope());
    return PushError(fmt::format("failed to find Symbol: `{0:s}`", symbol->Get()));
  }
  if (IsNull(result))
    return PushNext(Null());
  return PushNext(result);
}

auto Interpreter::VisitReturnInstr(ReturnInstr* instr) -> bool {
  const auto frame = PopStackFrame();
  if (!frame.HasReturnAddress())
    return Next();
  return Goto((Instruction*)frame.GetReturnAddressPointer());  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
}

auto Interpreter::VisitCastInstr(CastInstr* instr) -> bool {
  ASSERT(instr);
  const auto target_type = instr->GetTarget();
  ASSERT(target_type);
  {
    const auto top = GetStackTop();
    ASSERT(top);
    if ((*top)->GetType()->Equals(target_type)) {
      DVLOG(1000) << "skipping cast of " << (*top) << " to: " << target_type;
      return Next();
    }
  }

  const auto value = GetRuntime()->Pop();
  ASSERT(value);
  const auto current_type = value->GetType();
  ASSERT(current_type);
  DVLOG(1000) << "casting " << value << " to: " << target_type;
  if (target_type->Equals(Observable::GetClass()))
    return PushNext(Observable::New(value));
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

static inline auto Unary(const expr::UnaryOp op, Object* rhs) -> Object* {
  ASSERT(rhs);
  switch (op) {
    case expr::kNot:
      return Not(rhs);
    case expr::kCar:
      return Car(rhs);
    case expr::kCdr:
      return Cdr(rhs);
    case expr::kNull:
      return Bool::Box(IsNull(rhs));
    case expr::kNonnull:
      return Bool::Box(!IsNull(rhs));
    default:
      LOG(FATAL) << "invalid UnaryOp: " << op;
      return nullptr;
  }
}

auto Interpreter::VisitUnaryOpInstr(UnaryOpInstr* instr) -> bool {
  ASSERT(instr);
  const auto value = GetRuntime()->Pop();
  ASSERT(value);
  const auto result = Unary(instr->GetOp(), value);
  ASSERT(result);
  GetRuntime()->Push(result);
  return Next();
}

auto Interpreter::VisitGotoInstr(GotoInstr* instr) -> bool {
  ASSERT(instr);
  ASSERT(instr->HasTarget());
  return Goto(instr->GetTarget());
}

auto Interpreter::VisitThrowInstr(ThrowInstr* instr) -> bool {
  ASSERT(instr);
  GetRuntime()->Push(Error::New(GetRuntime()->Pop()));
  return Next();
}

auto Interpreter::VisitInvokeNativeInstr(InvokeNativeInstr* instr) -> bool {
  ASSERT(instr);
  const auto target = GetRuntime()->Pop();
  if (!target || !target->IsNativeProcedure())
    throw Exception(fmt::format("expected {0:s} to be a NativeProcedure.", target ? target->ToString() : "null"));
  const auto procedure = target->AsProcedure();
  ASSERT(procedure && procedure->IsNative());
  const auto runtime = GetRuntime();
  ASSERT(runtime);
  runtime->CallWithNArgs(procedure->AsNativeProcedure(), instr->GetNumberOfArgs());
  return Next();
}

auto Interpreter::VisitInvokeInstr(InvokeInstr* instr) -> bool {
  ASSERT(instr);
  const auto runtime = GetRuntime();
  ASSERT(runtime);
  const auto target = runtime->Pop();
  if (!IsProcedure(target))
    throw Exception(fmt::format("expected {0:s} to be a Procedure.", target ? target->ToString() : "null"));
  const auto procedure = target->AsProcedure();
  ASSERT(procedure);
  if (procedure->IsLambda()) {
    const auto lambda = procedure->AsLambda();
    ASSERT(lambda);
    if (!LambdaCompiler::Compile(lambda, runtime->GetCurrentScope())) {
      LOG(FATAL) << "failed to compile: " << lambda->ToString();
      return false;
    }
    runtime->Call(lambda);
  } else if (procedure->IsNativeProcedure()) {
    runtime->CallWithNArgs(procedure->AsNativeProcedure(), instr->GetNumberOfArgs());
  }
  return Next();
}

auto Interpreter::VisitInvokeDynamicInstr(InvokeDynamicInstr* instr) -> bool {
  ASSERT(instr);
  const auto target = GetRuntime()->Pop();
  ASSERT(target->IsSymbol());
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return Next();
}

auto Interpreter::VisitEvalInstr(EvalInstr* instr) -> bool {
  ASSERT(instr);
  const auto value = GetRuntime()->Pop();
  ASSERT(value && value->IsString());
  const auto result = GetRuntime()->Eval(String::Unbox(value));
  ASSERT(result);
  GetRuntime()->Push(result);
  return Next();
}

static inline auto GetTarget(const bool branch, instr::BranchInstr* instr) -> instr::EntryInstr* {
  if (branch)
    return instr->GetTrueTarget();
  if (instr->HasFalseTarget())
    return instr->GetFalseTarget();
  if (!instr->HasNext())
    return instr->GetJoin();
  return nullptr;
}

auto Interpreter::VisitBranchInstr(BranchInstr* instr) -> bool {
  ASSERT(instr);
  const auto test = GetRuntime()->Pop();
  ASSERT(test);
  const auto target = GetTarget(Truth(test), instr);
  return Goto(target ? target : instr->GetNext());
}

auto Interpreter::VisitInstanceOfInstr(InstanceOfInstr* instr) -> bool {
  ASSERT(instr);
  const auto type = instr->GetType();
  ASSERT(type);
  const auto stack_top = GetRuntime()->StackTop();
  if (!stack_top)
    return PushError(fmt::format("stack top is null, expected: {}", type->GetName()->Get()));
  if (!(*stack_top)->GetType()->IsInstanceOf(instr->GetType()))
    return PushError(fmt::format("unexpected stack top: {}, expected: {}", (*stack_top)->ToString(), type->GetName()->Get()));
  return Next();
}

static inline auto InstanceOf(Class* actual, Class* expected) -> Bool* {
  ASSERT(actual);
  ASSERT(expected);
  return Bool::Box(actual->IsInstanceOf(expected));
}

static inline auto InstanceOf(Datum* value, Datum* expected) -> Datum* {
  ASSERT(value);
  ASSERT(expected);
  if (scm::IsSymbol(expected)) {
    const auto cls = Class::FindClass(expected->AsSymbol());
    if (scm::IsNull(cls))
      return Error::New(fmt::format("failed to find class named `{}`", (*expected->AsSymbol())));
    return InstanceOf(value->GetClass(), cls);
  }
  ASSERT(expected->IsClass());
  return InstanceOf(value->GetClass(), expected->GetClass());
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
    case expr::kCons:
      return Pair::New(lhs, rhs);
    case expr::kInstanceOf:
      return InstanceOf(lhs, rhs);
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
  return Next();
}

auto Interpreter::VisitTargetEntryInstr(TargetEntryInstr* instr) -> bool {
  return Next();
}

auto Interpreter::VisitJoinEntryInstr(JoinEntryInstr* instr) -> bool {
  return Next();
}

auto Interpreter::VisitConstantInstr(ConstantInstr* instr) -> bool {
  const auto value = instr->GetValue();
  ASSERT(value);
  GetRuntime()->Push(value);
  return Next();
}

auto Interpreter::VisitStoreVariableInstr(StoreVariableInstr* instr) -> bool {
  const auto value = GetRuntime()->Pop();
  ASSERT(value);
  const auto symbol = instr->GetSymbol();
  ASSERT(symbol);
  if (!GetRuntime()->StoreSymbol(symbol, value)) {
    LOG(FATAL) << "failed to store symbol " << symbol << " to value: " << value;
    return Next();
  }
  return Next();
}

auto Interpreter::VisitGraphEntryInstr(GraphEntryInstr* instr) -> bool {
  return Next();
}

void Interpreter::ExecuteInstr(Instruction* instr) {
  ASSERT(instr);
  DVLOG(100) << "executing " << instr->ToString();
  LOG_IF(FATAL, !instr->Accept(this)) << "failed to execute: " << instr->ToString();
}

auto Interpreter::PushStackFrame(LocalScope* locals) -> StackFrame* {
  ASSERT(locals);
  const auto has_next = HasCurrentInstr() && !stack_.empty();
  const auto return_address =
      (uword)(has_next ? GetCurrentInstr()->GetNext() : UNALLOCATED);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  stack_.push(StackFrame(stack_.size(), locals, return_address));
  DVLOG(1000) << "pushed: " << stack_.top();
  return &stack_.top();
}

auto Interpreter::PopStackFrame() -> StackFrame {
  if (stack_.empty()) {
    LOG(WARNING) << "stack empty";
    return {};
  }
  ASSERT(!stack_.empty());
  const auto frame = stack_.top();
  stack_.pop();
  DVLOG(1000) << "popped: " << frame;
  return frame;
}

void Interpreter::Execute(instr::TargetEntryInstr* target, LocalScope* locals) {
  SetCurrentInstr(target);
  while (HasCurrentInstr()) {
    ExecuteInstr(GetCurrentInstr());
    if (GetRuntime()->HasError())
      break;
  }
}
}  // namespace scm