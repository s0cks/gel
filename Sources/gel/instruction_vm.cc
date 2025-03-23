#include "gel/assembler.h"
#include "gel/common.h"
#include "gel/expression.h"
#include "gel/flow_graph_compiler.h"
#include "gel/instruction.h"
#include "gel/native_procedure.h"
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
    case expr::kAdd:
      __ add();
      break;
    case expr::kSubtract:
      __ sub();
      break;
    case expr::kMultiply:
      __ mul();
      break;
    case expr::kDivide:
      __ div();
      break;
    case expr::kModulus:
      __ mod();
      break;
    case expr::kEq:
      __ eq();
      break;
    case expr::kBitAnd:
      __ band();
      break;
    case expr::kBitOr:
      __ bor();
      break;
    case expr::kBitXor:
      __ bxor();
      break;
    case expr::kGreaterThan:
      __ gt();
      break;
    case expr::kGreaterThanEqual:
      __ gte();
      break;
    case expr::kLessThan:
      __ lt();
      break;
    case expr::kLessThanEqual:
      __ lte();
      break;
    case expr::kCons:
      __ cons();
      break;
    case expr::kInstanceOf:
      __ instanceof();
      break;
    case expr::kShiftLeft:
      __ shl();
      break;
    case expr::kShiftRight:
      __ shr();
      break;
    default:
      LOG(FATAL) << "invalid BinaryOp: " << GetOp();
  }
}

COMPILE(UnaryOpInstr) {
  ASSERT(compiler);
  switch (GetOp()) {
    case expr::kNot:
      __ negate();
      break;
    case expr::kCar:
      __ car();
      break;
    case expr::kCdr:
      __ cdr();
      break;
    case expr::kNonnull:
      __ isnonnull();
      break;
    case expr::kNull:
      __ isnull();
      break;
    case expr::kBitNot:
      __ bnot();
      break;
    default:
      LOG(FATAL) << "invalid UnaryOp: " << GetOp();
  }
}

COMPILE(InvokeInstr) {
  ASSERT(compiler);
  ASSERT(GetProcedure()->IsLambda());
  __ invoke(GetProcedure()->AsLambda(), GetNumberOfArgs());
}

COMPILE(InvokeNativeInstr) {
  ASSERT(compiler);
  __ invokenative(GetNativeProcedure(), GetNumberOfArgs());
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
  __ stfield(GetField());
}

COMPILE(BranchInstr) {
  ASSERT(compiler);
  Label* tbranch = compiler->GetBlockLabel(GetTrueTarget());
  switch (condition_) {
    case kTrue:
      __ Branch(BranchCondition::kIsTrue, tbranch);
      break;
    case kNotTrue:
      __ Branch(BranchCondition::kIsFalse, tbranch);
      break;
    case kEqual:
      __ Branch(BranchCondition::kEquals, tbranch);
      break;
    case kNotEqual:
      __ Branch(BranchCondition::kNotEquals, tbranch);
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