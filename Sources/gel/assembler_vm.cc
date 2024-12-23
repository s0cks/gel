#include "gel/assembler.h"
#include "gel/bytecode.h"
#include "gel/memory_region.h"

namespace gel {
void Assembler::EmitLabel(Label* label) {
  ASSERT(label);
  if (label->IsBound()) {
    const auto offset = label->GetPos() - cbuffer().GetSize();
    buffer().Emit<word>(static_cast<word>(offset));
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
  const auto bound = static_cast<word>(cbuffer().GetSize() + sizeof(RawBytecode));
  while (label->IsLinked()) {
    const auto pos = label->GetLinkPos();
    const auto dest = static_cast<word>(bound - pos);
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
}  // namespace gel