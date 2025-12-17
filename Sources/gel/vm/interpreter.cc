#include "gel/interpreter.h"

#include <sstream>

#include "gel/common.h"
#include "gel/frontend/expr/binary_op.h"
#include "gel/frontend/expr/unary_op.h"

#include "gel/type/type.h"
#include "gel/type/object.h"
#include "gel/type/fn.h"

#include "gel/bytecode.h"
#include "gel/error.h"
#include "gel/local.h"
#include "gel/local_scope.h"
#include "gel/namespace.h"
#include "gel/vm/operation_stack.h"
#include "gel/runtime.h"
#include "gel/rx_object.h"

namespace gel {
#define TOP             (GetOperationStack()->GetTop())
#define POP             (GetOperationStack()->Pop())
#define POPN(N, Result) (GetOperationStack()->PopN((Result), (N), true));
#define PUSH(Value)     (GetOperationStack()->Push((Value)->IsNil() ? (Object*)Nil::Get() : (Value)))

auto Interpreter::GetCallStack() -> CallStack& {
  return runtime_->GetCallStack();
}

auto Interpreter::GetScope() const -> LocalScope* {
  return runtime_->GetScope();
}

void Interpreter::LoadLocal(const uword idx) {
  ASSERT(idx >= 0 && idx <= GetScope()->GetNumberOfLocals());
  auto scope = GetScope();
  do {
    ASSERT(scope);
    for (auto i = 0; i < scope->GetNumberOfLocals(); i++) {
      const auto local = GetScope()->GetLocalAt(i);
      ASSERT(local);
      if (local->GetIndex() == idx) {
        const auto value = local->HasValue() ? local->GetValue() : gel::Nil::Get();
        return PUSH(value);
      }
    }
    scope = scope->GetParent();
  } while (scope);

  std::stringstream ss;
  ss << "failed to load local #" << idx;
  return Throw(ss);
}

void Interpreter::StoreLocal(const uword idx) {
  const auto local = GetScope()->GetLocalAt(idx);
  if (!local) {
    LOG(ERROR) << "failed to find local #" << idx << " in current scope:";
    _PRINT_SCOPE_AT_LEVEL(google::ERROR, GetScope(), true);
    LOG(FATAL) << "";
  }
  const auto value = POP;
  ASSERT(value);
  local->SetValue((*value));
}

void Interpreter::Push(const Bytecode code) {
  switch (code.op()) {
    case Bytecode::kPushQ: {
      const auto value = NextObjectPointer();
      ASSERT(value);
      PUSH(value);
      return;
    }
    case Bytecode::kPushI: {
      const auto value = NextNumber();
      ASSERT(value);
      PUSH(value);
      return;
    }
    case Bytecode::kPushN: {
      const auto value = Nil::Get();
      ASSERT(value);
      PUSH(value);
      return;
    }
    case Bytecode::kPushF: {
      const auto value = Bool::False();
      ASSERT(value);
      PUSH(value);
      return;
    }
    case Bytecode::kPushT: {
      const auto value = Bool::True();
      ASSERT(value);
      PUSH(value);
      return;
    }
    default:
      LOG(FATAL) << "invalid Push instruction: " << code;
  }
}

void Interpreter::Jump(const uword target) {
  current_ = target;
}

void Interpreter::BranchTrue(const uword target) {
  const auto lhs = POP;
  LOG_IF(FATAL, !lhs) << "expected a value";
  if (gel::Truth((*lhs)))
    current_ = target;
}

void Interpreter::BranchFalse(const uword target) {
  const auto lhs = POP;
  LOG_IF(FATAL, !lhs) << "expected a value";
  if (!gel::Truth((*lhs)))
    current_ = target;
}

void Interpreter::BranchEq(const uword target) {
  const auto lhs = POP;
  LOG_IF(FATAL, !lhs) << "expected a lhs value";
  const auto rhs = POP;
  LOG_IF(FATAL, !rhs) << "expected a rhs value";
  const auto value = (*lhs)->Equals(*rhs);
  if (value)
    current_ = target;
}

void Interpreter::BranchNe(const uword target) {
  const auto lhs = POP;
  LOG_IF(FATAL, !lhs) << "expected a lhs value";
  const auto rhs = POP;
  LOG_IF(FATAL, !rhs) << "expected a rhs value";
  const auto value = (*lhs)->Equals(*rhs);
  if (!value)
    current_ = target;
}

void Interpreter::BranchGt(const uword target) {
  const auto lhs = POP;
  LOG_IF(FATAL, !lhs) << "expected a lhs value";
  const auto rhs = POP;
  LOG_IF(FATAL, !rhs) << "expected a rhs value";
  const auto value = (*lhs)->GreaterThan(*rhs);
  if (gel::Truth(value))
    current_ = target;
}

void Interpreter::BranchLt(const uword target) {
  const auto lhs = POP;
  LOG_IF(FATAL, !lhs) << "expected a lhs value";
  const auto rhs = POP;
  LOG_IF(FATAL, !rhs) << "expected a rhs value";
  const auto value = (*lhs)->LessThan(*rhs);
  if (gel::Truth(value))
    current_ = target;
}

void Interpreter::nop() {
  // do nothing
}

void Interpreter::bt() {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
}

void Interpreter::PopLookup() {
  const auto symbol = (*POP);
  LOG_IF(FATAL, !symbol || !symbol->IsSymbol()) << "expected " << (symbol ? symbol : Nil::Get()) << " to be a Symbol.";
  return Lookup(symbol->AsSymbol());
}

void Interpreter::Invoke(const Bytecode::Op op) {
  Fn* func = nullptr;
  if (op == Bytecode::kInvokeDynamic) {
    const auto next = (*POP);
    ASSERT(next);
    if (next->IsFn()) {
      func = next->AsFn();
    } else if (next->IsSymbol()) {
      const auto scope = GetScope();
      ASSERT(scope);
      const auto symbol = next->AsSymbol();
      ASSERT(symbol);
      LocalVariable* local = nullptr;
      if (!scope->Lookup(symbol, &local)) {
        if (!symbol->HasNamespace() || symbol->GetNamespace() != "gel") {
          const auto gel_symbol = Symbol::New("gel", symbol->GetSymbolName());
          if (!scope->Lookup(gel_symbol, &local)) {  // TODO: remove this double lookup?
            std::stringstream ss;
            ss << "failed to resolve symbol `" << symbol->GetFullyQualifiedName() << "`";
            return Throw(ss);
          }
        }
      }
      if (!local || !local->HasValue() || !local->GetValue()->IsFn()) {
        std::stringstream ss;
        ss << "failed to resolve symbol `" << symbol->GetFullyQualifiedName() << "` to Fn";
        return Throw(ss);
      }
      func = local->GetValue()->AsFn();
    } else {
      std::stringstream ss;
      ss << "expected " << next->ToString() << " to be a Symbol or Fn.";
      return Throw(ss);
    }
  } else {
    const auto next = NextObjectPointer();
    ASSERT(next && next->IsFn());
    func = next->AsFn();
  }
  ASSERT(func);
  const auto num_args = NextUWord();
  if (func->IsNative()) {
    ASSERT(op == Bytecode::kInvokeNative || op == Bytecode::kInvokeDynamic);
    return GetRuntime()->CallWithNArgs(*(func->AsNativeFn()), num_args);
  } else if (func->IsLambdaFn()) {
    ASSERT(op == Bytecode::kInvoke || op == Bytecode::kInvokeDynamic);
    return GetRuntime()->CallWithNArgs(*(func->AsLambdaFn()), num_args);
  }
  std::stringstream ss;
  ss << "cannot invoke: " << func->ToString();
  return Throw(ss);
}

void Interpreter::Throw() {
  const auto err = (*POP);
  ASSERT(err && err->IsError());
  throw Exception(err->AsError()->GetMessage()->Get());
}

void Interpreter::Throw(Error* error) {
  ASSERT(error);
  PUSH(error);
  return Throw();
}

void Interpreter::ExecBinaryOp(const Bytecode code) {
  ASSERT(code.IsBinaryOp());
  const auto rhs = (*POP);
  ASSERT(rhs);
  const auto lhs = (*POP);
  ASSERT(lhs);
  switch (code.op()) {
#define DEFINE_BINARY_OP(Name)         \
  case Bytecode::k##Name: {            \
    const auto value = lhs->Name(rhs); \
    ASSERT(value);                     \
    return PUSH(value);                \
  }
    FOR_EACH_BINARY_OP(DEFINE_BINARY_OP)
#undef DEFINE_BINARY_OP
    default:
      LOG(FATAL) << "invalid BinaryOp: " << code;
  }
}

void Interpreter::ExecUnaryOp(const Bytecode code) {
  ASSERT(code.IsUnaryOp());
  const auto value = (*POP);
  ASSERT(value);
  switch (code.op()) {
    case Bytecode::kNot: {
      const auto new_value = Bool::Box(!gel::Truth(value));
      ASSERT(new_value);
      PUSH(new_value);
      return;
    }
    case Bytecode::kCdr: {
      const auto new_value = gel::Cdr(value);
      ASSERT(new_value);
      PUSH(new_value);
      return;
    }
    case Bytecode::kCar: {
      const auto new_value = gel::Car(value);
      ASSERT(new_value);
      PUSH(new_value);
      return;
    }
    case Bytecode::kNull: {
      const auto new_value = Bool::Box(value->IsNil());
      ASSERT(new_value);
      PUSH(new_value);
      return;
    }
    case Bytecode::kNonnull: {
      const auto new_value = Bool::Box(!value->IsNil());
      ASSERT(new_value);
      PUSH(new_value);
      return;
    }
    case Bytecode::kBitNot: {
      if (!value->IsNumber())
        throw Exception("");
      const auto new_value = value->AsNumber()->BitNot();
      ASSERT(new_value);
      PUSH(new_value);
      return;
    }
    default:
      LOG(FATAL) << "invalid UnaryOp: " << code;
  }
}

void Interpreter::CheckInstance(Class* cls) {
  ASSERT(cls);
  const auto top = TOP;
  LOG_IF(FATAL, !top) << "expected " << Nil::Get() << " to be an instanceof " << cls;
  LOG_IF(FATAL, !(*top)->GetType()->IsInstanceOf(cls->AsClass()))
      << "expected " << (*top) << " to be an instanceof " << cls;
}

void Interpreter::Cast(Class* cls) {
  ASSERT(cls);
  const auto value = (*POP);
  ASSERT(value);
  if (cls->Equals(Observable::GetClass())) {
    const auto new_value = Observable::New(value);
    ASSERT(new_value);
    PUSH(new_value);
    return;
  }
}

void Interpreter::Lookup(Symbol* rhs) {
  ASSERT(rhs);
  const auto scope = GetScope();
  ASSERT(scope);
  LocalVariable* local = nullptr;
  if (!scope->Lookup(rhs, &local)) {
    if (!rhs->HasNamespace() || rhs->GetNamespace() != "gel") {
      const auto gel_symbol = Symbol::New("gel", rhs->GetSymbolName());
      if (scope->Lookup(gel_symbol, &local)) {  // TODO: remove this double lookup?
        ASSERT(local);
        const auto value = local->HasValue() ? local->GetValue() : Nil::Get();
        PUSH(value);
        return;
      }
    }
    std::stringstream ss;
    ss << "failed to resolve symbol `" << rhs->GetFullyQualifiedName() << "`";
    return Throw(ss);
  }
  ASSERT(local);
  const auto value = local->HasValue() ? local->GetValue() : Nil::Get();
  PUSH(value);
}

void Interpreter::Pop() {
  const auto value = POP;
  ASSERT(value);
}

void Interpreter::Dup() {
  return GetOperationStack()->Dup();
}

void Interpreter::Dup2() {
  return GetOperationStack()->Dup2();
}

void Interpreter::LoadField(Field* field) {
  ASSERT(field);
  const auto instance = POP;
  ASSERT(instance);
  const auto value = (*instance)->GetField(field);
  if (!value) {
    PUSH(Nil::Get());
    return;
  }
  PUSH(value);
}

void Interpreter::StoreField(Field* field) {
  ASSERT(field);
  const auto instance = POP;
  ASSERT(instance);
  const auto value = POP;
  ASSERT(value);
  (*instance)->SetField(field, (*value));
}

void Interpreter::New(Class* cls, const uword num_args) {
  ASSERT(cls);
  ObjectList args{};
  POPN(num_args, args);
  const auto value = cls->NewInstance(args);
  ASSERT(value);
  PUSH(value);
}

void Interpreter::NewList(const uword length) {
  ASSERT(length >= 0);
  Object* result = Nil::Get();
  for (uword idx = 0; idx < length; idx++) {
    const auto next = POP;
    LOG_IF(ERROR, !next) << "failed to pop " << length << "nth value for list.";
    result = Cons(next.value_or(Nil::Get()), result);
  }
  ASSERT(result);
  PUSH(result);
}

void Interpreter::Run(const uword start_address) {
  SetCurrentAddress(start_address);
  ASSERT(GetCurrentAddress() == start_address);
  while (true) {
    const auto current = GetCurrentAddress();
    const auto pos = (current - start_address);
    const auto op = NextOp();
    switch (op) {
      case Bytecode::kPushN:
      case Bytecode::kPushT:
      case Bytecode::kPushF:
      case Bytecode::kPushI:
      case Bytecode::kPushQ:
        Push(op);
        continue;
      case Bytecode::kPop:
        Pop();
        continue;
      case Bytecode::kDup:
        Dup();
        continue;
      case Bytecode::kDup2:
        Dup2();
        continue;
      case Bytecode::kLookup:
        PopLookup();
        continue;
      case Bytecode::kLoadLocal:
        LoadLocal(NextUWord());
        continue;
      case Bytecode::kLoadLocal0:
      case Bytecode::kLoadLocal1:
      case Bytecode::kLoadLocal2:
      case Bytecode::kLoadLocal3: {
        const auto idx = op - Bytecode::kLoadLocal0;
        LoadLocal(idx);
        continue;
      }
      case Bytecode::kStoreLocal:
        StoreLocal(NextUWord());
        continue;
      case Bytecode::kStoreLocal0:
      case Bytecode::kStoreLocal1:
      case Bytecode::kStoreLocal2:
      case Bytecode::kStoreLocal3:
        StoreLocal(op - Bytecode::kStoreLocal0);
        continue;
      case Bytecode::kInvoke:
      case Bytecode::kInvokeNative:
      case Bytecode::kInvokeDynamic:
        Invoke(op);
        continue;
      case Bytecode::kThrow:
        return Throw();
      case Bytecode::kCheckInstance: {
        const auto cls = NextObjectPointer();
        ASSERT(cls && cls->IsClass());
        CheckInstance(cls->AsClass());
        continue;
      }
      case Bytecode::kCast: {
        const auto cls = NextObjectPointer();
        ASSERT(cls && cls->IsClass());
        Cast(cls->AsClass());
        continue;
      }
      case Bytecode::kNop:
        nop();
        continue;
        // clang-format off
#define DECLARE_CASE(Name) \
  case Bytecode::k##Name:
      FOR_EACH_BINARY_OP(DECLARE_CASE)
        // clang-format on
        ExecBinaryOp(op);
        continue;
        // clang-format off
      FOR_EACH_UNARY_OP(DECLARE_CASE)
        // clang-format on
        ExecUnaryOp(op);
        continue;
#undef DECLARE_CASE
      case Bytecode::kRet: {
        GetCallStack()->SetReturnAddress(TOP.value_or(Nil::Get())->GetStartingAddress());
        return;
      }
      case Bytecode::kJump: {
        const auto target = start_address + (pos + NextWord());
        Jump(target);
        break;
      }
      case Bytecode::kBranchTrue:
      case Bytecode::kBranchFalse:
      case Bytecode::kBranchEq:
      case Bytecode::kBranchNeq:
      case Bytecode::kBranchGreaterThan:
      case Bytecode::kBranchLessThan: {
        const auto target = start_address + (pos + NextWord());
        Branch(static_cast<BranchCondition>(op - Bytecode::kBranchTrue), target);
        continue;
      }
      case Bytecode::kStoreField:
        StoreField(NextField());
        continue;
      case Bytecode::kLoadField:
        LoadField(NextField());
        continue;
      case Bytecode::kNew: {
        const auto cls = NextClass();
        ASSERT(cls);
        New(cls, NextUWord());
        continue;
      }
      case Bytecode::kList: {
        const auto length = NextUWord();
        NewList(length);
        continue;
      }
      case Bytecode::kInvalid:
      default:
        LOG(FATAL) << "invalid op: " << op;
    }
  }
}
}  // namespace gel
