#include "scheme/interpreter.h"

#include <glog/logging.h>

#include "scheme/common.h"
#include "scheme/expression.h"
#include "scheme/instruction.h"
#include "scheme/tracing.h"

namespace scm {
auto Interpreter::LoadSymbol(Symbol* symbol) -> bool {
  Datum* result = nullptr;
  if (!GetEnvironment()->Lookup(symbol, &result)) {
    LOG(ERROR) << "failed to get " << symbol;
    return false;
  }
  ASSERT(result);
  Push(result);
  return true;
}

void Interpreter::StoreSymbol(Symbol* symbol, Datum* value) {
  ASSERT(symbol);
  ASSERT(value);
  if (!GetEnvironment()->Put(symbol, value)) {
    LOG(ERROR) << "failed to set " << symbol << " to: " << value;
    return;
  }
  DLOG(INFO) << "set " << symbol << " to: " << value;
}

static inline auto Add(Datum* lhs, Datum* rhs) -> Datum* {
  ASSERT(lhs);
  ASSERT(rhs);
  return lhs->Add(rhs);
}

static inline auto Subtract(Datum* lhs, Datum* rhs) -> Datum* {
  ASSERT(lhs);
  ASSERT(rhs);
  return lhs->Sub(rhs);
}

static inline auto Multiply(Datum* lhs, Datum* rhs) -> Datum* {
  ASSERT(lhs);
  ASSERT(rhs);
  return lhs->Mul(rhs);
}

static inline auto Divide(Datum* lhs, Datum* rhs) -> Datum* {
  ASSERT(lhs);
  ASSERT(rhs);
  return lhs->Div(rhs);
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
  const auto symbol = instr->GetSymbol();
  ASSERT(symbol);
  StoreSymbol(symbol, value);
  return true;
}

auto Interpreter::VisitLoadVariableInstr(LoadVariableInstr* instr) -> bool {
  return LoadSymbol(instr->GetSymbol());
}

auto Interpreter::VisitReturnInstr(ReturnInstr* instr) -> bool {
  ASSERT(!stack_.empty());
  return true;
}

auto Interpreter::VisitBinaryOpInstr(BinaryOpInstr* instr) -> bool {
  const auto op = instr->GetOp();
  DLOG(INFO) << "op: " << op;
  const auto right = Pop();
  ASSERT(right);
  DLOG(INFO) << "right: " << right;
  const auto left = Pop();
  ASSERT(left);
  DLOG(INFO) << "left: " << left;
  switch (op) {
    case expr::kAdd:
      Push(Add(left, right));
      return true;
    case expr::kSub:
      Push(Subtract(left, right));
      return true;
    case expr::kMul:
      Push(Multiply(left, right));
      return true;
    case expr::kDiv:
      Push(Divide(left, right));
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
  DLOG(INFO) << "executing: " << instr->GetName();
  if (!instr->Accept(this))
    LOG(FATAL) << "failed to execute: " << instr->ToString();
}

auto Interpreter::Execute(EntryInstr* entry) -> Datum* {
  TRACE_BEGIN;
  ASSERT(entry);
  InstructionIterator iter(entry->GetFirstInstruction());
  while (iter.HasNext()) {
    ExecuteInstr(iter.Next());
  }

  if (stack_.empty())
    return Null::Get();

  const auto result = Pop();
  ASSERT(result);
  ASSERT(stack_.empty());
  return result;
}
}  // namespace scm