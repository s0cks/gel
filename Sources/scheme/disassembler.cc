#include "scheme/disassembler.h"

#include "scheme/common.h"
#include "scheme/expression.h"
#include "scheme/instruction.h"

namespace scm {
auto Disassembler::VisitGraphEntryInstr(instr::GraphEntryInstr* instr) -> bool {
  ASSERT(instr);
  const auto target = instr->GetNext();
  return target && target->Accept(this);
}

auto Disassembler::VisitTargetEntryInstr(instr::TargetEntryInstr* instr) -> bool {
  ASSERT(instr);
  instr::InstructionIterator iter(instr->GetNext());
  while (iter.HasNext()) {
    const auto next = iter.Next();
    if (!next || !next->Accept(this))
      return false;
  }
  return true;
}

auto Disassembler::VisitJoinEntryInstr(instr::JoinEntryInstr* instr) -> bool {
  ASSERT(instr);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return true;
}

auto Disassembler::VisitBinaryOpInstr(instr::BinaryOpInstr* instr) -> bool {
  ASSERT(instr);
  switch (instr->GetOp()) {
    case BinaryOp::kAdd:
      DLOG(INFO) << "add";
      break;
    case BinaryOp::kSubtract:
      DLOG(INFO) << "sub";
      break;
    case BinaryOp::kMultiply:
      DLOG(INFO) << "mul";
      break;
    case BinaryOp::kDivide:
      DLOG(INFO) << "div";
      break;
    case BinaryOp::kModulus:
      DLOG(INFO) << "mod";
      break;
    case BinaryOp::kEquals:
      DLOG(INFO) << "equals";
      break;
    case BinaryOp::kGreaterThan:
      DLOG(INFO) << "gt";
      break;
    case BinaryOp::kGreaterThanEqual:
      DLOG(INFO) << "gte";
      break;
    case BinaryOp::kLessThan:
      DLOG(INFO) << "lt";
      break;
    case BinaryOp::kLessThanEqual:
      DLOG(INFO) << "lte";
      break;
    case BinaryOp::kBinaryOr:
      DLOG(INFO) << "or";
      break;
    case BinaryOp::kBinaryAnd:
      DLOG(INFO) << "and";
      break;
    default:
      DLOG(INFO) << "unknown binary op.";
      return false;
  }
  return true;
}

auto Disassembler::VisitUnaryOpInstr(instr::UnaryOpInstr* instr) -> bool {
  ASSERT(instr);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return true;
}

auto Disassembler::VisitConstantInstr(instr::ConstantInstr* instr) -> bool {
  ASSERT(instr);
  const auto value = instr->GetValue();
  DLOG(INFO) << "ldc " << value->ToString();
  return true;
}

auto Disassembler::VisitReturnInstr(instr::ReturnInstr* instr) -> bool {
  ASSERT(instr);
  DLOG(INFO) << "ret";
  return true;
}

auto Disassembler::VisitLoadVariableInstr(instr::LoadVariableInstr* instr) -> bool {
  ASSERT(instr);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return true;
}

auto Disassembler::VisitStoreVariableInstr(instr::StoreVariableInstr* instr) -> bool {
  ASSERT(instr);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return true;
}

auto Disassembler::VisitInstanceOfInstr(instr::InstanceOfInstr* instr) -> bool {
  ASSERT(instr);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return true;
}

auto Disassembler::VisitThrowInstr(instr::ThrowInstr* instr) -> bool {
  ASSERT(instr);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return true;
}

auto Disassembler::VisitBranchInstr(instr::BranchInstr* instr) -> bool {
  ASSERT(instr);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return true;
}

auto Disassembler::VisitGotoInstr(instr::GotoInstr* instr) -> bool {
  ASSERT(instr);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return true;
}

auto Disassembler::VisitInvokeInstr(instr::InvokeInstr* instr) -> bool {
  ASSERT(instr);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return true;
}

auto Disassembler::VisitInvokeNativeInstr(instr::InvokeNativeInstr* instr) -> bool {
  ASSERT(instr);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return true;
}

auto Disassembler::VisitInvokeDynamicInstr(instr::InvokeDynamicInstr* instr) -> bool {
  ASSERT(instr);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return true;
}

auto Disassembler::VisitEvalInstr(instr::EvalInstr* instr) -> bool {
  ASSERT(instr);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return true;
}

auto Disassembler::Disassemble(GraphEntryInstr* instr) -> bool {
  ASSERT(instr);
  Disassembler disassembler;
  if (!instr->Accept(&disassembler)) {
    LOG(ERROR) << "failed to visit: " << instr->ToString();
    return false;
  }
  return true;
}
}  // namespace scm