#include "gel/instruction.h"

#include <sstream>
#include <string>

#include "gel/to_string_helper.h"

namespace gel::instr {
void Instruction::Append(Instruction* instr) {
  ASSERT(instr);
  if (HasNext())
    return GetNext()->Append(instr);
  SetNext(instr);
  instr->SetPrevious(this);
}

#define DEFINE_ACCEPT(Name)                            \
  auto Name::Accept(InstructionVisitor* vis) -> bool { \
    ASSERT(vis);                                       \
    return vis->Visit##Name(this);                     \
  }
FOR_EACH_INSTRUCTION(DEFINE_ACCEPT)
#undef DEFINE_ACCEPT

auto NewInstr::ToString() const -> std::string {
  ToStringHelper<NewInstr> helper;
  helper.AddField("target", GetTarget());
  return helper;
}

auto LoadVariableInstr::ToString() const -> std::string {
  std::stringstream ss;
  ss << "LoadVariableInstr(";
  ss << "symbol=" << GetSymbol()->ToString();
  ss << ")";
  return ss.str();
}

auto EvalInstr::ToString() const -> std::string {
  std::stringstream ss;
  ss << "EvalInstr(";
  ss << "value=" << GetValue()->ToString();
  ss << ")";
  return ss.str();
}

auto StoreVariableInstr::ToString() const -> std::string {
  std::stringstream ss;
  ss << "StoreVariableInstr(";
  ss << "symbol=" << GetSymbol()->ToString() << ", ";
  ss << "value=" << GetValue()->ToString();
  ss << ")";
  return ss.str();
}

auto ConstantInstr::ToString() const -> std::string {
  std::stringstream ss;
  ss << "ConstantInstr(";
  ss << "value=" << GetValue()->ToString();
  ss << ")";
  return ss.str();
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
  std::stringstream ss;
  ss << "GraphEntryInstr()";
  return ss.str();
}

auto TargetEntryInstr::ToString() const -> std::string {
  std::stringstream ss;
  ss << "TargetEntryInstr(";
  ss << "block_id=" << GetBlockId();
  ss << ")";
  return ss.str();
}

auto JoinEntryInstr::ToString() const -> std::string {
  std::stringstream ss;
  ss << "JoinEntryInstr(";
  ss << ")";
  return ss.str();
}

auto ReturnInstr::ToString() const -> std::string {
  std::stringstream ss;
  ss << "ReturnInstr(";
  if (HasValue())
    ss << "value=" << GetValue()->ToString();
  ss << ")";
  return ss.str();
}

auto BinaryOpInstr::ToString() const -> std::string {
  std::stringstream ss;
  ss << "BinaryOpInstr(";
  ss << "op=" << GetOp();
  ss << ")";
  return ss.str();
}

auto BranchInstr::ToString() const -> std::string {
  std::stringstream ss;
  ss << "BranchInstr(";
  ss << "test=" << GetTest()->ToString() << ", ";
  ss << "true_target=" << GetTrueTarget()->ToString() << ", ";
  if (HasFalseTarget())
    ss << "false_target=" << GetFalseTarget()->ToString() << ", ";
  ss << "join=" << GetJoin()->ToString();
  ss << ")";
  return ss.str();
}

auto GotoInstr::ToString() const -> std::string {
  std::stringstream ss;
  ss << "GotoInstr(";
  ss << "target=" << GetTarget()->ToString();
  ss << ")";
  return ss.str();
}

auto UnaryOpInstr::ToString() const -> std::string {
  std::stringstream ss;
  ss << "UnaryOpInstr(";
  ss << "op=" << GetOp() << ", ";
  ss << "value=" << GetValue()->ToString();
  ss << ")";
  return ss.str();
}

auto ThrowInstr::ToString() const -> std::string {
  std::stringstream ss;
  ss << "ThrowInstr(";
  ss << "value=" << GetValue()->ToString();
  ss << ")";
  return ss.str();
}

auto InvokeInstr::ToString() const -> std::string {
  ToStringHelper<InvokeInstr> helper;
  helper.AddField("target", GetTarget());
  helper.AddField("num_args", GetNumberOfArgs());
  return helper;
}

auto InvokeDynamicInstr::ToString() const -> std::string {
  std::stringstream ss;
  ss << "InvokeDynamicInstr(";
  ss << "target=" << GetTarget()->ToString();
  ss << ")";
  return ss.str();
}

auto InvokeNativeInstr::ToString() const -> std::string {
  ToStringHelper<InvokeNativeInstr> helper;
  helper.AddField("target", GetTarget());
  helper.AddField("num_args", GetNumberOfArgs());
  return helper;
}

auto InstanceOfInstr::ToString() const -> std::string {
  std::stringstream ss;
  ss << "InstanceOfInstr(";
  ss << "value=" << GetValue()->ToString();
  ss << ")";
  return ss.str();
}

auto CastInstr::ToString() const -> std::string {
  ToStringHelper<CastInstr> helper;
  helper.AddField("value", GetValue());
  helper.AddField("target", GetTarget());
  return helper;
}
}  // namespace gel::instr