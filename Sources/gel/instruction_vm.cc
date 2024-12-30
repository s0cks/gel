#include "gel/common.h"
#include "gel/expression.h"
#include "gel/flow_graph_compiler.h"
#include "gel/instruction.h"
#include "gel/native_procedure.h"
// TODO: add platform guard

namespace gel::ir {
#define __ compiler->assembler()->
void GraphEntryInstr::Compile(FlowGraphCompiler* compiler) {
  ASSERT(compiler);
  // do nothing
}

void TargetEntryInstr::Compile(FlowGraphCompiler* compiler) {
  ASSERT(compiler);
  // do nothing
}

void JoinEntryInstr::Compile(FlowGraphCompiler* compiler) {
  ASSERT(compiler);
  // do nothing
}

void ConstantInstr::Compile(FlowGraphCompiler* compiler) {
  ASSERT(compiler);
  __ Push(GetValue());
}

void StoreLocalInstr::Compile(FlowGraphCompiler* compiler) {
  ASSERT(compiler);
  __ StoreLocal(GetLocal()->GetIndex());
}

void LoadLocalInstr::Compile(FlowGraphCompiler* compiler) {
  ASSERT(compiler);
  __ LoadLocal(GetLocal()->GetIndex());
}

void BinaryOpInstr::Compile(FlowGraphCompiler* compiler) {
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

void UnaryOpInstr::Compile(FlowGraphCompiler* compiler) {
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

void InvokeInstr::Compile(FlowGraphCompiler* compiler) {
  ASSERT(compiler);
  ASSERT(GetProcedure()->IsLambda());
  __ invoke(GetProcedure()->AsLambda(), GetNumberOfArgs());
}

void InvokeNativeInstr::Compile(FlowGraphCompiler* compiler) {
  ASSERT(compiler);
  __ invokenative(GetNativeProcedure(), GetNumberOfArgs());
}

void InvokeDynamicInstr::Compile(FlowGraphCompiler* compiler) {
  ASSERT(compiler);
  __ invokedynamic(GetNumberOfArgs());
}

void LookupInstr::Compile(FlowGraphCompiler* compiler) {
  ASSERT(compiler);
  __ lookup();
}

void ReturnInstr::Compile(FlowGraphCompiler* compiler) {
  ASSERT(compiler);
  __ ret();
}

void LoadFieldInstr::Compile(FlowGraphCompiler* compiler) {
  ASSERT(compiler);
  __ ldfield(GetField());
}

void StoreFieldInstr::Compile(FlowGraphCompiler* compiler) {
  ASSERT(compiler);
  __ stfield(GetField());
}

void BranchInstr::Compile(FlowGraphCompiler* compiler) {
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

void GotoInstr::Compile(FlowGraphCompiler* compiler) {
  ASSERT(compiler);
  auto& target_info = compiler->GetBlockInfo(GetTarget()->GetBlockId());
  __ jmp(&target_info.label);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
}

void ThrowInstr::Compile(FlowGraphCompiler* compiler) {
  ASSERT(compiler);
  __ th();
}

void CastInstr::Compile(FlowGraphCompiler* compiler) {
  ASSERT(compiler);
  __ CastTo(GetTarget());
}

void NewInstr::Compile(FlowGraphCompiler* compiler) {
  ASSERT(compiler);
  __ New(GetTarget(), GetNumberOfArgs());
}

void InstanceOfInstr::Compile(FlowGraphCompiler* compiler) {
  ASSERT(compiler);
  __ CheckInstance(GetType());
}
#undef __
}  // namespace gel::ir