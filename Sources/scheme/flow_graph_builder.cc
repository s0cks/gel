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
  const auto graph_entry = GraphEntryInstr::New(GetNextBlockId());
  ASSERT(graph_entry);

  const auto target_entry = TargetEntryInstr::New(GetNextBlockId());
  ASSERT(target_entry);
  SetCurrentBlock(target_entry);
  EffectVisitor for_effect(this);
  if (!GetProgram()->Accept(&for_effect)) {
    LOG(ERROR) << "failed to visit: " << GetProgram()->ToString();
    return nullptr;  // TODO: free entry
  }
  AppendFragment(target_entry, for_effect);

  const auto last = target_entry->GetLastInstruction();
  ASSERT(last);
  if (!last->IsReturnInstr() && last->IsDefinition())
    Instruction::Link(last, new ReturnInstr(reinterpret_cast<Definition*>(last)));  // NOLINT

  SetGraphEntry(graph_entry);
  graph_entry->Append(target_entry);
  graph_entry->AddDominated(target_entry);
  return new FlowGraph(graph_entry);
}

auto EffectVisitor::VisitEval(EvalExpr* expr) -> bool {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto EffectVisitor::VisitCallProc(CallProcExpr* expr) -> bool {
  for (auto idx = 0; idx < expr->GetNumberOfChildren(); idx++) {
    const auto arg = expr->GetChildAt(idx);
    ASSERT(arg);
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

auto EffectVisitor::VisitCond(CondExpr* expr) -> bool {
  ASSERT(expr);
  const auto join = JoinEntryInstr::New(GetOwner()->GetNextBlockId());

  // process conseq
  const auto conseq_target = TargetEntryInstr::New(GetOwner()->GetNextBlockId());
  ValueVisitor for_conseq(GetOwner());
  if (!expr->GetConseq()->Accept(&for_conseq)) {
    LOG(ERROR) << "failed to visit conseq for cond: " << expr->ToString();
    return false;
  }
  AppendFragment(conseq_target, for_conseq);
  conseq_target->Append(GotoInstr::New(join));
  GetOwner()->GetCurrentBlock()->AddDominated(conseq_target);

  // process alt
  const auto alt_target = TargetEntryInstr::New(GetOwner()->GetNextBlockId());
  ValueVisitor for_alt(GetOwner());
  if (expr->HasAlternate()) {
    if (!expr->GetAlternate()->Accept(&for_alt)) {
      LOG(ERROR) << "failed to visit alternate for cond: " << expr->ToString();
      return false;
    }
    AppendFragment(alt_target, for_alt);
    alt_target->Append(GotoInstr::New(join));
    GetOwner()->GetCurrentBlock()->AddDominated(alt_target);
  }

  // process test
  ValueVisitor for_test(GetOwner());
  if (!expr->GetTest()->Accept(&for_test)) {
    LOG(ERROR) << "failed to visit test for cond: " << expr->ToString();
    return false;
  }
  Append(for_test);

  const auto branch = expr->HasAlternate() ? BranchInstr::New(for_test.GetValue(), conseq_target, alt_target, join)
                                           : BranchInstr::New(for_test.GetValue(), conseq_target, nullptr, join);
  ASSERT(branch);
  ReturnDefinition(branch);
  SetExitInstr(join);

  GetOwner()->GetCurrentBlock()->AddDominated(join);
  return true;
}

auto EffectVisitor::VisitDefine(DefineExpr* expr) -> bool {
  ASSERT(expr);
  // process value
  const auto value = expr->GetValue();
  ASSERT(value);
  ValueVisitor for_value(GetOwner());
  if (!value->Accept(&for_value)) {
    LOG(FATAL) << "failed to determine value for: " << expr->ToString();
    return false;
  }
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