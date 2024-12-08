#include "gel/interpreter.h"

#include <algorithm>
#include <ranges>

#include "gel/array.h"
#include "gel/common.h"
#include "gel/error.h"
#include "gel/expression.h"
#include "gel/instruction.h"
#include "gel/lambda.h"
#include "gel/local.h"
#include "gel/local_scope.h"
#include "gel/macro.h"
#include "gel/module.h"
#include "gel/namespace.h"
#include "gel/native_procedure.h"
#include "gel/object.h"
#include "gel/platform.h"
#include "gel/runtime.h"
#include "gel/script.h"

namespace gel {
Interpreter::Interpreter(Runtime* runtime) :
  InstructionVisitor(),
  runtime_(runtime) {}

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

auto Interpreter::VisitLoadLocalInstr(LoadLocalInstr* instr) -> bool {
  ASSERT(instr);
  const auto locals = GetRuntime()->GetScope();
  ASSERT(locals);
  LocalVariable* local = nullptr;
  if (!locals->Lookup(instr->GetLocal()->GetName(), &local)) {
    LOG(ERROR) << "failed to find local named `" << instr->GetLocal()->GetName() << "` in LocalScope:";
    LocalScopePrinter::Print<google::ERROR, false>(locals, __FILE__, __LINE__);
    LOG(FATAL) << "";
  }
  ASSERT(local);
  const auto result = local->GetValue();
  if (IsNull(result))
    return PushNext(Null());
  return PushNext(result);
}

auto Interpreter::VisitReturnInstr(ReturnInstr* instr) -> bool {
  const auto frame = GetRuntime()->PopStackFrame();
  GetRuntime()->PopScope();
  if (frame.HasReturnAddress()) {
    SetCurrentInstr((Instruction*)frame.GetReturnAddress());  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
    return false;
  }
  SetCurrentInstr(instr->GetNext());
  return false;
}

auto Interpreter::VisitNewInstr(NewInstr* instr) -> bool {
  ASSERT(instr);
  const auto type = instr->GetTarget();
  ASSERT(type);
  const auto runtime = GetRuntime();
  ASSERT(runtime);
  ObjectList args;
  runtime->PopN(args, instr->GetNumberOfArgs(), true);
  if (instr->GetTarget()->Equals(Array<Object>::GetClass())) {
    const auto length = gel::ToLong(args[0]);
    ASSERT(length);
    return PushNext(Array<Object>::New(Long::Unbox(length)));
  }
#define DEFINE_NEW_TYPE(Name) \
  if (type->Is<Name>())       \
    return PushNext(Name::New(args));
  FOR_EACH_TYPE(DEFINE_NEW_TYPE)
#undef DEFINE_NEW_TYPE
  return false;
}

auto Interpreter::VisitCastInstr(CastInstr* instr) -> bool {
  ASSERT(instr);
  const auto target = instr->GetTarget();
  ASSERT(target);
  {
    const auto top = GetStackTop();
    ASSERT(top);
    if ((*top)->GetType()->IsInstanceOf(target)) {
      DVLOG(1000) << "skipping cast of " << (*top) << " to: " << target;
      return Next();
    }
  }

  const auto value = GetRuntime()->Pop();
  ASSERT(value);
  const auto current_type = value->GetType();
  ASSERT(current_type);
  DVLOG(1000) << "casting " << value << " to: " << target;
  if (target->Is<Observable>()) {
    return PushNext(Observable::New(value));
  } else if (target->Is<String>()) {
    return PushNext(String::ValueOf(value));
  } else if (target->Is<Subject>()) {
    return PushNext(ToSubject(value));
  }
  return PushError(fmt::format("Cannot cast `{}` to {}", (*value), (*target->GetName())));
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
  GetRuntime()->CallWithNArgs(target->AsNativeProcedure(), instr->GetNumberOfArgs());
  DVLOG(1000) << "current instr: " << GetCurrentInstr()->ToString();
  DVLOG(1000) << "next instr: " << GetCurrentInstr()->GetNext()->ToString();
  return Next();
}

auto Interpreter::VisitInvokeInstr(InvokeInstr* instr) -> bool {
  ASSERT(instr);
  const auto runtime = GetRuntime();
  ASSERT(runtime);
  const auto target = runtime->Pop();
  if (!IsProcedure(target))
    throw Exception(fmt::format("expected {0:s} to be a Procedure.", target ? target->ToString() : "null"));
  if (target->IsLambda()) {
    runtime->CallWithNArgs(target->AsLambda(), instr->GetNumberOfArgs());
  } else if (target->IsNativeProcedure()) {
    runtime->CallWithNArgs(target->AsNativeProcedure(), instr->GetNumberOfArgs());
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

static inline auto GetTarget(const bool branch, ir::BranchInstr* instr) -> ir::EntryInstr* {
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
  const auto runtime = GetRuntime();
  ASSERT(runtime);
  const auto stack_top = runtime->StackTop();
  if (!stack_top) {
    if (!instr->IsStrict())
      return PushError(fmt::format("stack top is null, expected: {}", type->GetName()->Get()));
    runtime->Pop();
    return PushNext(Bool::False());
  }
  if (!(*stack_top)->GetType()->IsInstanceOf(instr->GetType())) {
    if (instr->IsStrict())
      return PushError(fmt::format("unexpected stack top: {}, expected: {}", (*stack_top)->ToString(), type->GetName()->Get()));
    runtime->Pop();
    return PushNext(Bool::False());
  }
  if (!instr->IsStrict()) {
    runtime->Pop();
    return PushNext(Bool::True());
  }
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
  if (gel::IsSymbol(expected)) {
    const auto cls = Class::FindClass(expected->AsSymbol());
    if (gel::IsNull(cls))
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
      return Bool::Box(lhs->GetType()->IsInstanceOf(rhs->GetType()));
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
  GetRuntime()->PushStackFrame(instr);
  return Next();
}

auto Interpreter::VisitJoinEntryInstr(JoinEntryInstr* instr) -> bool {
  GetRuntime()->PopStackFrame();
  return Next();
}

auto Interpreter::VisitConstantInstr(ConstantInstr* instr) -> bool {
  const auto value = instr->GetValue();
  ASSERT(value);
  GetRuntime()->Push(value);
  return Next();
}

auto Interpreter::VisitStoreLocalInstr(StoreLocalInstr* instr) -> bool {
  const auto local = instr->GetLocal();
  ASSERT(local);
  const auto value = GetRuntime()->Pop();
  ASSERT(value);
  const auto locals = GetRuntime()->GetScope();
  ASSERT(locals);
  local->SetValue(value);
  if (!locals->Add(local)) {
    LOG(ERROR) << "failed to add " << (*local) << " to:";
    PRINT_SCOPE(ERROR, locals);
    LOG(FATAL) << "";
  }
  return Next();
}

auto Interpreter::VisitGraphEntryInstr(GraphEntryInstr* instr) -> bool {
  return Next();
}

auto Interpreter::ExecuteInstr(Instruction* instr) -> bool {
  ASSERT(instr);
  DVLOG(10) << "executing " << instr->ToString();
  return instr->Accept(this);
}

void Interpreter::Run() {
  while (HasCurrentInstr()) {
    if (!ExecuteInstr(GetCurrentInstr()))
      break;
  }
}
}  // namespace gel