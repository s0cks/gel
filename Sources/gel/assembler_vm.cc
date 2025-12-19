#include "assembler.h"
#include "assembler_base.h"
#include "bytecode.h"
#include "common.h"
#include "memory_region.h"
#include "platform.h"
#include "region.h"

namespace gel {
void Assembler::EmitLabel(Label* label) {
  ASSERT(label);
  if (label->IsBound()) {
    const auto offset = label->GetPos() - cbuffer().GetSize();
    buffer().Emit<uword>(offset);
  } else {
    EmitLabelLink(label);
  }
}

void Assembler::EmitLabelLink(Label* label) {
  ASSERT(label);
  const auto pos = cbuffer().GetSize();
  buffer().Emit<word>(label->pos_);
  label->LinkTo(static_cast<word>(pos));
}

auto Assembler::Assemble() const -> Region {
  MemoryRegion region(cbuffer().GetSize(), MemoryRegion::kReadWrite);
  region.CopyFrom(cbuffer().GetStartingAddress(), cbuffer().GetSize());
  return {region};
}

void Assembler::Bind(Label* label) {
  ASSERT(label);
  const auto bound = static_cast<word>(cbuffer().GetSize() + sizeof(Bytecode::Op));
  while (label->IsLinked()) {
    const auto pos = label->GetLinkPos();
    const auto dest = bound - pos;
    const auto next = buffer().LoadAt<word>(pos);
    buffer().StoreAt<word>(pos, dest);
    label->pos_ = next;
  }
  label->BindTo(bound);
}

void Assembler::Jump(Bytecode::Op op, Label* label) {
  ASSERT(label);
  if (label->IsBound()) {
    const auto offset = static_cast<word>(label->GetPos() - cbuffer().GetSize());
    ASSERT(offset <= 0);
    EmitOp(op);
    buffer().Emit<word>(offset);
  } else {
    EmitOp(op);
    EmitLabelLink(label);
  }
}

void Assembler::Branch(BranchCondition cond, Label* label) {
  ASSERT(label);
  switch (cond) {
    case BranchCondition::kEquals:
      EmitOp(Bytecode::kBranchEq);
      break;
    case BranchCondition::kNotEquals:
      EmitOp(Bytecode::kBranchNeq);
      break;
    case BranchCondition::kGreaterThan:
      EmitOp(Bytecode::kBranchGreaterThan);
      break;
    case BranchCondition::kLessThan:
      EmitOp(Bytecode::kBranchLessThan);
      break;
    case BranchCondition::kIsTrue:
      EmitOp(Bytecode::kBranchTrue);
      break;
    case BranchCondition::kIsFalse:
      EmitOp(Bytecode::kBranchFalse);
      break;
  }
  if (label->IsBound()) {
    const auto offset = static_cast<word>(label->GetPos() - cbuffer().GetSize());
    ASSERT(offset <= 0);
    buffer().Emit<word>(offset);
  } else {
    EmitLabelLink(label);
  }
}
}  // namespace gel
