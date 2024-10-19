#include "scheme/interpreter.h"

#include <glog/logging.h>

#include "scheme/ast.h"
#include "scheme/common.h"
#include "scheme/instruction.h"
#include "scheme/tracing.h"

namespace scm {
auto Interpreter::LoadVariable(Variable* var) -> Datum* {
  ASSERT(var);
  DLOG(INFO) << "loading " << var->ToString() << "....";
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return nullptr;
}

static inline auto Add(Datum* lhs, Datum* rhs) -> Datum* {
  ASSERT(lhs);
  ASSERT(rhs);
  return lhs->Add(rhs);
}

auto Interpreter::VisitGraphEntryInstr(GraphEntryInstr* instr) -> bool {
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return true;
}

auto Interpreter::VisitCallProcInstr(CallProcInstr* instr) -> bool {
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return true;
}

auto Interpreter::VisitConstantInstr(ConstantInstr* instr) -> bool {
  const auto value = instr->GetValue();
  ASSERT(value);
  Push(value);
  return true;
}

auto Interpreter::VisitStoreVariableInstr(StoreVariableInstr* instr) -> bool {
  const auto value = Pop();
  ASSERT(value);
  const auto variable = instr->GetVariable();
  ASSERT(variable);
  DLOG(INFO) << "setting " << variable->ToString() << " := " << value->ToString();
  Push(value);
  return true;
}

auto Interpreter::VisitLoadVariableInstr(LoadVariableInstr* instr) -> bool {
  const auto variable = instr->GetVariable();
  const auto value = LoadVariable(variable);
  Push(value);
  return true;
}

auto Interpreter::VisitReturnInstr(ReturnInstr* instr) -> bool {
  ASSERT(!stack_.empty());
  return true;
}

auto Interpreter::VisitBinaryOpInstr(BinaryOpInstr* instr) -> bool {
  const auto op = instr->GetOp();
  DLOG(INFO) << "op: " << op;
  const auto left = Pop();
  ASSERT(left);
  DLOG(INFO) << "left: " << left;
  const auto right = Pop();
  ASSERT(right);
  DLOG(INFO) << "right: " << right;
  switch (op) {
    case ast::kAddOp:
      Push(Add(left, right));
      return true;
    default:
      LOG(ERROR) << "invalid BinaryOp: " << op;
      return false;
  }
}

void Interpreter::ExecuteInstr(Instruction* instr) {
  ASSERT(instr);
  TRACE_SECTION(ExecuteInstr);
  TRACE_TAG(instr->GetName());
  DLOG(INFO) << "executing: " << instr->ToString();
  if (!instr->Accept(this))
    LOG(FATAL) << "failed to execute: " << instr->ToString();
}

auto Interpreter::Execute(EntryInstr* entry) -> Datum* {
  TRACE_BEGIN;
  ASSERT(entry);
  InstructionIterator iter(entry->GetFirstInstruction());
  while (iter.HasNext()) {
    const auto current = iter.Next();
    ASSERT(current);
    ExecuteInstr(current);
  }

  ASSERT(!stack_.empty());
  const auto result = Pop();
  ASSERT(result);
  ASSERT(stack_.empty());
  return result;
}
}  // namespace scm