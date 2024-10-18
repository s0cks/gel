#include "scheme/instruction.h"

#include <sstream>

namespace scm {
auto LoadVariableInstr::ToString() const -> std::string {
  std::stringstream ss;
  ss << "LoadVariableInstr(";
  ss << "variable=" << GetVariable()->ToString();
  ss << ")";
  return ss.str();
}

auto StoreVariableInstr::ToString() const -> std::string {
  std::stringstream ss;
  ss << "StoreVariableInstr(";
  ss << "variable=" << GetVariable()->ToString() << ", ";
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
  InstructionIterator iter(const_cast<EntryInstr*>(this));  // NOLINT
  while (iter.HasNext()) last = iter.Next();
  return last;
}

auto GraphEntryInstr::GetFirstInstruction() const -> Instruction* {
  return GetNext();
}

auto GraphEntryInstr::ToString() const -> std::string {
  std::stringstream ss;
  ss << "GraphEntryInstr()";
  return ss.str();
}

auto CallProcInstr::ToString() const -> std::string {
  std::stringstream ss;
  ss << "CallProcInstr(";
  ss << "procedure=" << GetProcedure()->ToString();
  ss << ")";
  return ss.str();
}

auto ReturnInstr::ToString() const -> std::string {
  std::stringstream ss;
  ss << "ReturnInstr(";
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
}  // namespace scm