#include "gel/instruction.h"

#include <string>
#include <vector>

#include "gel/common.h"
#include "gel/to_string_helper.h"

namespace gel::ir {
void Input::Bind(Definition* rhs) {
  ASSERT(rhs);
  RemoveFromList();
  SetDefinition(rhs);
  rhs->AddInput(this);
}

void Input::RemoveFromList() {
  const auto next = GetNext();
  const auto defn = GetDefinition();
  if (this == defn->GetInputUseList()) {
    defn->SetInputUseList(next);
    if (next)
      next->SetPrevious(nullptr);
  } else {
    const auto previous = GetPrevious();
    if (previous)
      previous->SetNext(next);
    if (next)
      next->SetPrevious(previous);
  }

  previous_ = next_ = nullptr;
}

static inline auto IsMarked(EntryInstr* blk, std::vector<EntryInstr*>& preorder) -> bool {
  ASSERT(blk);
  const auto index = blk->GetPreorderNum();
  return index >= 0 && index < preorder.size() && preorder[index] == blk;
}

auto EntryInstr::DiscoverBlocks(EntryInstr* predecessor, std::vector<EntryInstr*>& preorder, std::vector<word>& parent)
    -> bool {
  if (IsMarked(this, preorder)) {
    AddPredecessor(predecessor);
    return false;
  }

  ClearPredecessors();
  if (predecessor != nullptr)
    AddPredecessor(predecessor);
  const auto parent_num = predecessor == nullptr ? -1 : predecessor->GetPreorderNum();
  parent.push_back(parent_num);
  SetPreorderNum(static_cast<word>(preorder.size()));
  preorder.push_back(this);

  Instruction* last = this;
  InstructionIterator iter(last);
  while (iter.HasNext()) {
    last = iter.Next();
  }
  SetLastInstruction(last);
  return true;
}

auto GraphEntryInstr::GetSuccessorAt(const uword idx) const -> EntryInstr* {
  return GetTarget();
}

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
  helper.AddField("local", GetLocal());
  return helper;
}

auto StoreLocalInstr::ToString() const -> std::string {
  ToStringHelper<StoreLocalInstr> helper;
  helper.AddField("local", GetLocal());
  helper.AddField("value", GetValue());
  return helper;
}

auto ConstantInstr::ToString() const -> std::string {
  ToStringHelper<ConstantInstr> helper;
  helper.AddField("value", GetValue());
  return helper;
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

auto LetEntryInstr::ToString() const -> std::string {
  ToStringHelper<LetEntryInstr> helper{};
  helper.AddField("id", GetBlockId());
  return helper;
}

auto TargetEntryInstr::ToString() const -> std::string {
  ToStringHelper<TargetEntryInstr> helper;
  helper.AddField("block_id", GetBlockId());
  return helper;
}

auto NewListInstr::ToString() const -> std::string {
  ToStringHelper<NewListInstr> helper;
  helper.AddField("length", GetLength());
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

auto LoadFieldInstr::ToString() const -> std::string {
  ToStringHelper<LoadFieldInstr> helper;
  helper.AddField("instance", GetInstance());
  helper.AddField("field", GetField());
  return helper;
}

auto StoreFieldInstr::ToString() const -> std::string {
  ToStringHelper<StoreFieldInstr> helper;
  helper.AddField("field", GetField());
  helper.AddField("instance", GetInstance());
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
