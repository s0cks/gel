#include "gel/assembler.h"
#include "gel/assembler_base.h"
#include "gel/bytecode.h"
#include "gel/common.h"
#include "gel/expr/expression.h"
#include "gel/flow_graph_compiler.h"
#include "gel/instruction.h"
// TODO: add platform guard

namespace gel::ir {
#define COMPILE(Name) void Name::Compile(FlowGraphCompiler* compiler)

#define __            compiler->assembler()->

COMPILE(GraphEntryInstr) {
  ASSERT(compiler);
  auto& info = compiler->GetBlockInfo(this);
  __ Bind(&info.label);
}

COMPILE(TargetEntryInstr) {
  ASSERT(compiler);
  auto& info = compiler->GetBlockInfo(this);
  __ Bind(&info.label);
  // do nothing
}

COMPILE(LetEntryInstr) {
  ASSERT(compiler);
  auto& info = compiler->GetBlockInfo(this);
  __ Bind(&info.label);
}

COMPILE(JoinEntryInstr) {
  ASSERT(compiler);
  auto& info = compiler->GetBlockInfo(this);
  __ Bind(&info.label);
  if (IsLetJoin()) {
    DLOG(INFO) << "joining from let";
  }
}

COMPILE(ConstantInstr) {
  ASSERT(compiler);
  __ Push(GetValue());
}

COMPILE(StoreLocalInstr) {
  ASSERT(compiler);
  __ StoreLocal(GetLocal()->GetIndex());
}

COMPILE(LoadLocalInstr) {
  ASSERT(compiler);
  __ LoadLocal(GetLocal()->GetIndex());
}

COMPILE(BinaryOpInstr) {
  ASSERT(compiler);
  switch (GetOp()) {
    case BinaryOp::kAdd:
      __ add();
      break;
    case BinaryOp::kSubtract:
      __ sub();
      break;
    case BinaryOp::kMultiply:
      __ mul();
      break;
    case BinaryOp::kDivide:
      __ div();
      break;
    case BinaryOp::kModulus:
      __ mod();
      break;
    case BinaryOp::kEq:
      __ eq();
      break;
    case BinaryOp::kBitAnd:
      __ band();
      break;
    case BinaryOp::kBitOr:
      __ bor();
      break;
    case BinaryOp::kBitXor:
      __ bxor();
      break;
    case BinaryOp::kGreaterThan:
      __ gt();
      break;
    case BinaryOp::kGreaterThanEqual:
      __ gte();
      break;
    case BinaryOp::kLessThan:
      __ lt();
      break;
    case BinaryOp::kLessThanEqual:
      __ lte();
      break;
    case BinaryOp::kCons:
      __ cons();
      break;
    case BinaryOp::kInstanceOf:
      __ instanceof();
      break;
    case BinaryOp::kShiftLeft:
      __ shl();
      break;
    case BinaryOp::kShiftRight:
      __ shr();
      break;
    default:
      LOG(FATAL) << "invalid BinaryOp: " << GetOp();
  }
}

COMPILE(UnaryOpInstr) {
  ASSERT(compiler);
  switch (GetOp()) {
    case UnaryOp::kNot:
      __ negate();
      break;
    case UnaryOp::kCar:
      __ car();
      break;
    case UnaryOp::kCdr:
      __ cdr();
      break;
    case UnaryOp::kNonnull:
      __ isnonnull();
      break;
    case UnaryOp::kNull:
      __ isnull();
      break;
    case UnaryOp::kBitNot:
      __ bnot();
      break;
    default:
      LOG(FATAL) << "invalid UnaryOp: " << GetOp();
  }
}

COMPILE(InvokeInstr) {
  ASSERT(compiler);
  ASSERT(GetFn()->IsLambdaFn());
  __ invoke(GetFn()->AsLambdaFn(), GetNumberOfArgs());
}

COMPILE(InvokeNativeInstr) {
  ASSERT(compiler);
  __ invokenative(GetNativeFn(), GetNumberOfArgs());
}

COMPILE(InvokeDynamicInstr) {
  ASSERT(compiler);
  __ invokedynamic(GetNumberOfArgs());
}

COMPILE(LookupInstr) {
  ASSERT(compiler);
  __ lookup();
}

COMPILE(ReturnInstr) {
  ASSERT(compiler);
  __ ret();
}

COMPILE(LoadFieldInstr) {
  ASSERT(compiler);
  __ ldfield(GetField());
}

COMPILE(StoreFieldInstr) {
  ASSERT(compiler);
  __ dup2();
  __ stfield(GetField());
}

COMPILE(BranchInstr) {
  ASSERT(compiler);
  Label* branch = compiler->GetBlockLabel(GetTrueTarget());
  switch (condition_) {
    case kTrue:
      __ Branch(BranchCondition::kIsTrue, branch);
      break;
    case kNotTrue:
      __ Branch(BranchCondition::kIsFalse, branch);
      break;
    case kEqual:
      __ Branch(BranchCondition::kEquals, branch);
      break;
    case kNotEqual:
      __ Branch(BranchCondition::kNotEquals, branch);
      break;
    default:
      LOG(FATAL) << "invalid condition: " << condition_;
  }
}

COMPILE(GotoInstr) {
  ASSERT(compiler);
  auto& target_info = compiler->GetBlockInfo(GetTarget()->GetBlockId());
  __ jmp(&target_info.label);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
}

COMPILE(ThrowInstr) {
  ASSERT(compiler);
  __ th();
}

COMPILE(CastInstr) {
  ASSERT(compiler);
  __ CastTo(GetTarget());
}

COMPILE(NewInstr) {
  ASSERT(compiler);
  __ New(GetTarget(), GetNumberOfArgs());
}

COMPILE(NewListInstr) {
  ASSERT(compiler);
  __ list(GetLength());
}

COMPILE(InstanceOfInstr) {
  ASSERT(compiler);
  __ CheckInstance(GetType());
}
#undef __
}  // namespace gel::ir