#include "scheme/flow_graph_builder.h"

#include <glog/logging.h>

#include "scheme/common.h"
#include "scheme/expression.h"
#include "scheme/instruction.h"

namespace scm {
static inline auto AppendFragment(EntryInstr* entry, EffectVisitor& vis) -> Instruction* {
  if (vis.IsEmpty())
    return entry;
  Instruction::Link(entry, vis.GetEntryInstr());
  return vis.GetExitInstr();
}

auto FlowGraphBuilder::BuildGraph() -> FlowGraph* {
  const auto entry = GraphEntryInstr::New();
  SetGraphEntry(entry);

  EffectVisitor for_effect(this);
  if (!GetProgram()->Accept(&for_effect)) {
    LOG(ERROR) << "failed to visit: " << GetProgram()->ToString();
    return nullptr;  // TODO: free entry
  }
  AppendFragment(entry, for_effect);

  const auto last = entry->GetLastInstruction();
  ASSERT(last);
  if (!last->IsReturnInstr() && last->IsDefinition())
    Instruction::Link(last, new ReturnInstr(reinterpret_cast<Definition*>(last)));  // NOLINT
  return new FlowGraph(entry);
}

auto EffectVisitor::VisitEval(EvalExpr* expr) -> bool {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto EffectVisitor::VisitCallProc(CallProcExpr* expr) -> bool {
  for (auto idx = 0; idx < expr->GetNumberOfChildren(); idx++) {
    const auto arg = expr->GetChildAt(idx);
    ASSERT(value);
    ValueVisitor for_value(GetOwner());
    LOG_IF(ERROR, !arg->Accept(&for_value)) << "failed to determine value for: " << expr->ToString();
    Append(for_value);
  }
  const auto symbol = expr->GetSymbol();
  ASSERT(symbol);
  ReturnDefinition(CallProcInstr::New(symbol));
  return true;
}

auto EffectVisitor::VisitSymbol(SymbolExpr* expr) -> bool {
  ASSERT(expr);
  const auto symbol = expr->GetSymbol();
  ASSERT(symbol);
  ReturnDefinition(LoadVariableInstr::New(symbol));
  return true;
}

auto EffectVisitor::VisitBegin(BeginExpr* expr) -> bool {
  ASSERT(expr);
  uint64_t idx = 0;
  while (IsOpen() && (idx < expr->GetNumberOfChildren())) {
    const auto child = expr->GetChildAt(idx++);
    ASSERT(child);
    EffectVisitor vis(GetOwner());
    if (!child->Accept(&vis))
      break;
    Append(vis);
    if (!IsOpen())
      break;
  }
  return true;
}

auto EffectVisitor::VisitDefine(DefineExpr* expr) -> bool {
  ASSERT(expr);
  // process value
  const auto value = expr->GetValue();
  ASSERT(value);
  ValueVisitor for_value(GetOwner());
  LOG_IF(ERROR, !value->Accept(&for_value)) << "failed to determine value for: " << expr->ToString();
  Append(for_value);
  const auto symbol = expr->GetSymbol();
  ASSERT(symbol);
  ReturnDefinition(StoreVariableInstr::New(symbol, for_value.GetValue()));
  return true;
}

auto EffectVisitor::VisitLiteral(LiteralExpr* p) -> bool {
  ASSERT(p);
  ReturnDefinition(new ConstantInstr(p->GetValue()));
  return true;
}

auto EffectVisitor::VisitBinaryOp(BinaryOpExpr* expr) -> bool {
  ASSERT(expr);

  ASSERT(expr->HasLeft());
  ValueVisitor for_left(GetOwner());
  if (!expr->GetLeft()->Accept(&for_left))
    return false;
  Append(for_left);

  ASSERT(expr->HasRight());
  ValueVisitor for_right(GetOwner());
  if (!expr->GetRight()->Accept(&for_right))
    return false;
  Append(for_right);

  ReturnDefinition(BinaryOpInstr::New(expr->GetOp()));
  return true;
}
}  // namespace scm