#include "gel/common.h"
#include "gel/expr/expression.h"
#include "gel/flow_graph_compiler.h"
#include "gel/instruction.h"
#include "gel/native_procedure.h"
// TODO: add platform guard

namespace gel::ir {
#define COMPILE(Name) void Name::Compile(FlowGraphCompiler* compiler)

#define __            compiler->assembler()->
COMPILE(GraphEntryInstr) {
  ASSERT(compiler);
  // do nothing
}

COMPILE(TargetEntryInstr) {
  ASSERT(compiler);
  // do nothing
}

COMPILE(JoinEntryInstr) {
  ASSERT(compiler);
  // do nothing
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
    case expr::kEquals:
      __ eq();
      break;
    case expr::kBinaryAnd:
      __ band();
      break;
    case expr::kBinaryOr:
      __ bor();
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
  Label* fbranch = HasFalseTarget() ? compiler->GetBlockLabel(GetFalseTarget()) : nullptr;
  Label* join = compiler->GetBlockLabel(GetJoin());
  switch (condition_) {
    case kTrue:
      __ jnz(HasFalseTarget() ? fbranch : join);
      break;
    case kEqual:
      __ jne(HasFalseTarget() ? fbranch : join);
      break;
    default:
      LOG(FATAL) << "invalid condition: " << condition_;
  }
  // tbranch
  {
    __ Bind(tbranch);
    ir::InstructionIterator iter(GetTrueTarget());
    while (iter.HasNext()) {
      const auto next = iter.Next();
      ASSERT(next);
      next->Compile(compiler);
    }
  }
  // fbranch
  if (HasFalseTarget()) {
    __ Bind(fbranch);
    ir::InstructionIterator iter(GetFalseTarget());
    while (iter.HasNext()) {
      const auto next = iter.Next();
      ASSERT(next);
      next->Compile(compiler);
    }
  }
  // join
  {
    __ Bind(join);
    ir::InstructionIterator iter(GetJoin());
    while (iter.HasNext()) {
      const auto next = iter.Next();
      ASSERT(next);
      next->Compile(compiler);
    }
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