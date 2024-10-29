#include "scheme/flow_graph_builder.h"

#include <glog/logging.h>

#include "scheme/common.h"
#include "scheme/expression.h"
#include "scheme/instruction.h"
#include "scheme/lambda.h"
#include "scheme/procedure.h"

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
  ValueVisitor for_effect(this);
  if (!GetExpr()->Accept(&for_effect)) {
    LOG(ERROR) << "failed to visit: " << GetExpr()->ToString();
    return nullptr;  // TODO: free entry
  }
  AppendFragment(target_entry, for_effect);

  const auto last = target_entry->GetLastInstruction();
  ASSERT(last);
  if (!last->IsReturnInstr() && last->IsDefinition())
    last->Append(new ReturnInstr(dynamic_cast<instr::Definition*>(last)));  // NOLINT

  SetGraphEntry(graph_entry);
  graph_entry->Append(target_entry);
  graph_entry->AddDominated(target_entry);
  return new FlowGraph(graph_entry);
}

auto EffectVisitor::VisitEvalExpr(EvalExpr* expr) -> bool {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto EffectVisitor::VisitCallProcExpr(CallProcExpr* expr) -> bool {
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

auto EffectVisitor::VisitSymbolExpr(SymbolExpr* expr) -> bool {
  ASSERT(expr);
  const auto symbol = expr->GetSymbol();
  ASSERT(symbol);
  ReturnDefinition(LoadVariableInstr::New(symbol));
  return true;
}

auto EffectVisitor::VisitModuleDef(ModuleDef* expr) -> bool {
  ASSERT(expr);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto EffectVisitor::VisitImportDef(expr::ImportDef* expr) -> bool {
  ASSERT(expr);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto EffectVisitor::VisitBeginExpr(BeginExpr* expr) -> bool {
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

auto EffectVisitor::VisitConsExpr(ConsExpr* expr) -> bool {
  ASSERT(expr);

  ValueVisitor for_car(GetOwner());
  {
    ASSERT(expr->HasCar());
    if (!expr->GetCar()->Accept(&for_car)) {
      LOG(FATAL) << "failed to visit cons car: " << expr->GetCar()->ToString();
      return false;
    }
  }
  Append(for_car);

  ValueVisitor for_cdr(GetOwner());
  {
    ASSERT(expr->HasCdr());
    if (!expr->GetCdr()->Accept(&for_cdr)) {
      LOG(FATAL) << "failed to visit cons cdr: " << expr->GetCdr()->ToString();
      return false;
    }
  }
  Append(for_cdr);

  ReturnDefinition(ConsInstr::New(for_car.GetValue(), for_cdr.GetValue()));
  return true;
}

auto EffectVisitor::VisitCondExpr(CondExpr* expr) -> bool {
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

auto EffectVisitor::VisitLambdaExpr(LambdaExpr* expr) -> bool {
  const auto lambda = Lambda::New(expr->GetArgs(), expr->GetBody());
  ASSERT(lambda);
  ReturnDefinition(ConstantInstr::New(lambda));
  return true;
}

auto EffectVisitor::VisitUnaryExpr(expr::UnaryExpr* expr) -> bool {
  ASSERT(expr && expr->HasValue());
  ValueVisitor for_value(GetOwner());
  if (!expr->GetValue()->Accept(&for_value)) {
    LOG(FATAL) << "failed to visit value for: " << expr->ToString();
    return false;
  }
  Append(for_value);
  ReturnDefinition(instr::UnaryOpInstr::New(expr->GetOp(), for_value.GetValue()));
  return true;
}

auto EffectVisitor::VisitLocalDef(LocalDef* expr) -> bool {
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
  Add(StoreVariableInstr::New(symbol, for_value.GetValue()));
  return true;
}

auto EffectVisitor::VisitLiteralExpr(LiteralExpr* p) -> bool {
  ASSERT(p);
  ReturnDefinition(ConstantInstr::New(p->GetValue()));
  return true;
}

auto EffectVisitor::VisitBinaryOpExpr(BinaryOpExpr* expr) -> bool {
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

auto EffectVisitor::VisitSetExpr(expr::SetExpr* expr) -> bool {
  ASSERT(expr && expr->HasValue());
  ValueVisitor for_value(GetOwner());
  if (!expr->GetValue()->Accept(&for_value)) {
    LOG(FATAL) << "failed to visit SetExpr value: " << expr->GetValue()->ToString();
    return false;
  }
  Append(for_value);
  Add(StoreVariableInstr::New(expr->GetSymbol(), for_value.GetValue()));
  return true;
}
}  // namespace scm