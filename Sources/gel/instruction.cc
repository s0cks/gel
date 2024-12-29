#include "gel/instruction.h"

#include <sstream>
#include <string>

#include "gel/to_string_helper.h"

namespace gel::ir {
void Instruction::Append(Instruction* instr) {
  ASSERT(instr);
  if (HasNext())
    return GetNext()->Append(instr);
  SetNext(instr);
  instr->SetPrevious(this);
}

#define DEFINE_ACCEPT(Name)                                 \
  auto Name##Instr::Accept(InstructionVisitor* vis)->bool { \
    ASSERT(vis);                                            \
    return vis->Visit##Name##Instr(this);                   \
  }
FOR_EACH_INSTRUCTION(DEFINE_ACCEPT)
#undef DEFINE_ACCEPT

auto NewInstr::ToString() const -> std::string {
  ToStringHelper<NewInstr> helper;
  helper.AddField("target", GetTarget());
  return helper;
}

auto LoadLocalInstr::ToString() const -> std::string {
  ToStringHelper<LoadLocalInstr> helper;
  helper.AddField("local", *(GetLocal()));
  return helper;
}

auto StoreLocalInstr::ToString() const -> std::string {
  ToStringHelper<StoreLocalInstr> helper;
  helper.AddField("local", *(GetLocal()));
  helper.AddField("value", GetValue());
  return helper;
}

auto ConstantInstr::ToString() const -> std::string {
  ToStringHelper<ConstantInstr> helper;
  helper.AddField("value", GetValue());
  return helper;
}

auto EntryInstr::GetLastInstruction() const -> Instruction* {
  Instruction* last = nullptr;
  InstructionIterator iter(GetFirstInstruction());  // NOLINT
  while (iter.HasNext()) last = iter.Next();
  return last;
}

auto EntryInstr::VisitDominated(InstructionVisitor* vis) -> bool {
  ASSERT(vis);
  for (const auto& dominated : dominated_) {
    if (!dominated->Accept(vis))
      return false;
  }
  return true;
}

auto GraphEntryInstr::GetFirstInstruction() const -> Instruction* {
  const auto next = GetNext();
  ASSERT(next);
  return next->IsEntryInstr() ? next->AsEntryInstr()->GetFirstInstruction() : next;
}

auto GraphEntryInstr::ToString() const -> std::string {
  ToStringHelper<GraphEntryInstr> helper;
  helper.AddField("block_id", GetBlockId());
  helper.AddField("target", GetTarget());
  return helper;
}

auto TargetEntryInstr::ToString() const -> std::string {
  ToStringHelper<TargetEntryInstr> helper;
  helper.AddField("block_id", GetBlockId());
  return helper;
}

auto JoinEntryInstr::ToString() const -> std::string {
  ToStringHelper<JoinEntryInstr> helper;
  helper.AddField("block_id", GetBlockId());
  return helper;
}

auto LookupInstr::ToString() const -> std::string {
  ToStringHelper<LookupInstr> helper;
  helper.AddField("symbol", GetSymbol());
  return helper;
}

auto ReturnInstr::ToString() const -> std::string {
  ToStringHelper<ReturnInstr> helper;
  if (HasValue())
    helper.AddField("value", GetValue());
  return helper;
}

auto BinaryOpInstr::ToString() const -> std::string {
  ToStringHelper<BinaryOpInstr> helper;
  helper.AddField("op", GetOp());
  helper.AddField("left", GetLeft());
  helper.AddField("right", GetRight());
  return helper;
}

auto BranchInstr::ToString() const -> std::string {
  ToStringHelper<BranchInstr> helper;
  helper.AddField("true_target", GetTrueTarget());
  if (HasFalseTarget())
    helper.AddField("false_target", GetFalseTarget());
  helper.AddField("join", GetJoin());
  return helper;
}

auto GotoInstr::ToString() const -> std::string {
  ToStringHelper<GotoInstr> helper;
  helper.AddField("target", GetTarget());
  return helper;
}

auto UnaryOpInstr::ToString() const -> std::string {
  ToStringHelper<BranchInstr> helper;
  helper.AddField("op", GetOp());
  helper.AddField("value", GetValue());
  return helper;
}

auto ThrowInstr::ToString() const -> std::string {
  ToStringHelper<ThrowInstr> helper;
  helper.AddField("value", GetValue());
  return helper;
}

auto InvokeInstr::ToString() const -> std::string {
  ToStringHelper<InvokeInstr> helper;
  helper.AddField("target", GetTarget());
  helper.AddField("num_args", GetNumberOfArgs());
  return helper;
}

auto InvokeDynamicInstr::ToString() const -> std::string {
  ToStringHelper<InvokeDynamicInstr> helper;
  helper.AddField("target", GetTarget());
  helper.AddField("num_args", GetNumberOfArgs());
  return helper;
}

auto InvokeNativeInstr::ToString() const -> std::string {
  ToStringHelper<InvokeNativeInstr> helper;
  helper.AddField("target", GetTarget());
  helper.AddField("num_args", GetNumberOfArgs());
  return helper;
}

auto InstanceOfInstr::ToString() const -> std::string {
  ToStringHelper<InstanceOfInstr> helper;
  helper.AddField("type", GetType());
  helper.AddField("value", GetValue());
  helper.AddField("strict", IsStrict());
  return helper;
}

auto CastInstr::ToString() const -> std::string {
  ToStringHelper<CastInstr> helper;
  helper.AddField("value", GetValue());
  helper.AddField("target", GetTarget());
  return helper;
}
}  // namespace gel::ir