#include "gel/interpreter.h"

#include <algorithm>
#include <exception>
#include <ranges>
#include <stdexcept>

#include "gel/array.h"
#include "gel/bytecode.h"
#include "gel/common.h"
#include "gel/disassembler.h"
#include "gel/error.h"
#include "gel/event_loop.h"
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
#define TOP             (GetExecutionStack()->StackTop())
#define POP             (GetExecutionStack()->Pop())
#define POPN(N, Result) (GetExecutionStack()->PopN((Result), (N), true));
#define PUSH(Value)     (GetExecutionStack()->Push(gel::IsNull((Value)) ? Null() : (Value)))

auto Interpreter::GetExecutionStack() -> ExecutionStack* {
  return runtime_->GetExecutionStack();
}

auto Interpreter::GetScope() const -> LocalScope* {
  return runtime_->GetScope();
}

void Interpreter::LoadLocal(const uword idx) {
  ASSERT(idx >= 0 && idx <= GetScope()->GetNumberOfLocals());
  const auto local = GetScope()->GetLocalAt(idx);
  ASSERT(local && local->HasValue());
  return PUSH(local->GetValue());
}

void Interpreter::StoreLocal(const uword idx) {
  ASSERT(idx >= 0 && idx <= GetScope()->GetNumberOfLocals());
  const auto local = GetScope()->GetLocalAt(idx);
  ASSERT(local);
  const auto value = POP;
  ASSERT(value);
  local->SetValue(value);
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
      const auto value = NextLong();
      ASSERT(value);
      PUSH(value);
      return;
    }
    case Bytecode::kPushN: {
      const auto value = Null();
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

void Interpreter::Jump(const Bytecode code, const uword target) {
  switch (code.op()) {
    case Bytecode::kJnz: {
      const auto value = POP;
      ASSERT(value);
      if (!gel::Truth(value))
        current_ = target;
      return;
    }
    case Bytecode::kJne: {
      const auto rhs = POP;
      ASSERT(rhs);
      const auto lhs = POP;
      ASSERT(lhs);
      if (!lhs->Equals(rhs))
        current_ = target;
      return;
    }
    case Bytecode::kJump:
      current_ = target;
      return;
    default:
      LOG(FATAL) << "invalid Jump bytecode: " << code;
  }
}

void Interpreter::nop() {
  // do nothing
}

void Interpreter::bt() {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
}

void Interpreter::PopLookup() {
  const auto symbol = POP;
  LOG_IF(FATAL, !symbol || !symbol->IsSymbol()) << "expected " << (symbol ? symbol : Null()) << " to be a Symbol.";
  return Lookup(symbol->AsSymbol());
}

void Interpreter::Invoke(const Bytecode::Op op) {
  const auto func = op != Bytecode::kInvokeDynamic ? NextObjectPointer() : POP;
  ASSERT(func && func->IsProcedure());
  const auto num_args = NextUWord();
  if (func->IsNativeProcedure()) {
    ASSERT(op == Bytecode::kInvokeNative || op == Bytecode::kInvokeDynamic);
    return GetRuntime()->CallWithNArgs(func->AsNativeProcedure(), num_args);
  } else if (func->IsLambda()) {
    ASSERT(op == Bytecode::kInvoke || op == Bytecode::kInvokeDynamic);
    return GetRuntime()->CallWithNArgs(func->AsLambda(), num_args);
  }
  const auto error = Error::New(fmt::format("cannot invoke {}", (*func)));
  ASSERT(error);
  PUSH(error);
  return Throw();
}

void Interpreter::Throw() {
  const auto err = POP;
  ASSERT(err && err->IsError());
  throw std::runtime_error(err->AsError()->GetMessage()->Get());
}

void Interpreter::ExecBinaryOp(const Bytecode code) {
  ASSERT(code.IsBinaryOp());
  const auto rhs = POP;
  ASSERT(rhs);
  const auto lhs = POP;
  ASSERT(lhs);
  switch (code.op()) {
    case Bytecode::kAdd: {
      const auto value = lhs->Add(rhs);
      ASSERT(value);
      return PUSH(value);
    }
    case Bytecode::kSubtract: {
      const auto value = lhs->Sub(rhs);
      ASSERT(value);
      return PUSH(value);
    }
    case Bytecode::kDivide: {
      const auto value = lhs->Div(rhs);
      ASSERT(value);
      return PUSH(value);
    }
    case Bytecode::kMultiply: {
      const auto value = lhs->Mul(rhs);
      ASSERT(value);
      return PUSH(value);
    }
    case Bytecode::kModulus: {
      const auto value = lhs->Mod(rhs);
      ASSERT(value);
      return PUSH(value);
    }
    case Bytecode::kEquals: {
      const auto value = Bool::Box(lhs->Equals(rhs));
      ASSERT(value);
      return PUSH(value);
    }
    case Bytecode::kBinaryAnd: {
      const auto value = lhs->And(rhs);
      ASSERT(value);
      return PUSH(value);
    }
    case Bytecode::kBinaryOr: {
      const auto value = lhs->Or(rhs);
      ASSERT(value);
      return PUSH(value);
    }
    case Bytecode::kLessThan: {
      const auto comparison = lhs->Compare(rhs);
      const auto value = Bool::Box(comparison < 0);
      ASSERT(value);
      return PUSH(value);
    }
    case Bytecode::kLessThanEqual: {
      const auto comparison = lhs->Compare(rhs);
      const auto value = Bool::Box(comparison <= 0);
      ASSERT(value);
      return PUSH(value);
    }
    case Bytecode::kGreaterThan: {
      const auto comparison = lhs->Compare(rhs);
      const auto value = Bool::Box(comparison > 0);
      ASSERT(value);
      return PUSH(value);
    }
    case Bytecode::kGreaterThanEqual: {
      const auto comparison = lhs->Compare(rhs);
      const auto value = Bool::Box(comparison >= 0);
      ASSERT(value);
      return PUSH(value);
    }
    case Bytecode::kCons: {
      const auto value = gel::Cons(lhs, rhs);
      ASSERT(value);
      return PUSH(value);
    }
    case Bytecode::kInstanceOf: {
      ASSERT(rhs->IsClass());
      const auto value = Bool::Box(lhs->GetType()->IsInstanceOf(rhs->AsClass()));
      ASSERT(value);
      return PUSH(value);
    }
    default:
      LOG(FATAL) << "invalid BinaryOp: " << code;
  }
}

void Interpreter::ExecUnaryOp(const Bytecode code) {
  ASSERT(code.IsUnaryOp());
  const auto value = POP;
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
      const auto new_value = Bool::Box(gel::IsNull(value));
      ASSERT(new_value);
      PUSH(new_value);
      return;
    }
    case Bytecode::kNonnull: {
      const auto new_value = Bool::Box(!gel::IsNull(value));
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
  LOG_IF(FATAL, !top) << "expected " << Null() << " to be an instanceof " << cls;
  LOG_IF(FATAL, !(*top)->GetType()->IsInstanceOf(cls->AsClass())) << "expected " << (*top) << " to be an instanceof " << cls;
}

void Interpreter::Cast(Class* cls) {
  ASSERT(cls);
  const auto value = POP;
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
  LOG_IF(FATAL, !scope->Lookup(rhs, &local)) << "failed to resolve " << rhs;
  const auto value = local->HasValue() ? local->GetValue() : Null();
  PUSH(value);
}

void Interpreter::Pop() {
  const auto value = POP;
  ASSERT(value);
}

void Interpreter::Dup() {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
}

void Interpreter::New(Class* cls, const uword num_args) {
  ASSERT(cls);
  ObjectList args{};
  POPN(num_args, args);
  const auto value = cls->NewInstance(args);
  ASSERT(value);
  PUSH(value);
}

void Interpreter::Run(const uword address) {
  SetCurrentAddress(address);
  ASSERT(GetCurrentAddress() == address);
  while (true) {
    const auto start_address = GetCurrentAddress();
    const auto pos = (start_address - address);
    const auto op = NextBytecode();
    switch (op.op()) {
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
        Invoke(op.op());
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
        const auto event_loop = GetThreadEventLoop();
        ASSERT(event_loop);
        while (event_loop->Run(UV_RUN_NOWAIT) != 0);  // do nothing
        return;
      }
      case Bytecode::kJump:
      case Bytecode::kJz:
      case Bytecode::kJnz:
      case Bytecode::kJeq:
      case Bytecode::kJne: {
        const auto offset = NextWord();
        Jump(op, address + (pos + offset));
        continue;
      }
      case Bytecode::kNew: {
        const auto cls = NextClass();
        ASSERT(cls);
        New(cls, NextUWord());
        continue;
      }
      case Bytecode::kInvalid:
      default:
        LOG(FATAL) << "invalid op: " << op;
    }
  }
}
}  // namespace gel