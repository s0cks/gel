#include "scheme/flow_graph_builder.h"

#include <glog/logging.h>

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
  LOG_IF(ERROR, !GetProgram()->Accept(&for_effect)) << "failed to visit: " << GetProgram()->ToString();
  AppendFragment(entry, for_effect);

  const auto last = entry->GetLastInstruction();
  ASSERT(last);
  DLOG(INFO) << "last: " << last->ToString();
  if (!last->IsReturnInstr() && last->IsDefinition())
    Instruction::Link(last, new ReturnInstr(reinterpret_cast<Definition*>(last)));  // NOLINT
  return new FlowGraph(entry);
}

auto EffectVisitor::VisitProgram(ast::Program* p) -> bool {
  uint64_t idx = 0;
  while (IsOpen() && (idx < p->GetTotalNumberOfForms())) {
    const auto form = p->GetFormAt(idx++);
    ASSERT(form);

    if (form->IsDefinition()) {
      EffectVisitor vis(GetOwner());
      if (!form->Accept(&vis))
        break;
      Append(vis);
    } else if (form->IsExpression()) {
      ValueVisitor vis(GetOwner());
      if (!form->Accept(&vis))
        break;
      Append(vis);
    }
    if (!IsOpen())
      break;
  }
  return true;
}

auto EffectVisitor::VisitBeginDef(ast::BeginDef* p) -> bool {
  ASSERT(p);
  uint64_t idx = 0;
  while (IsOpen() && (idx < p->GetTotalNumberOfDefinitions())) {
    const auto def = p->GetDefinitionAt(idx++);
    ASSERT(def);
    EffectVisitor vis(GetOwner());
    if (!def->Accept(&vis))
      break;
    Append(vis);
    if (!IsOpen())
      break;
  }
  return true;
}

auto EffectVisitor::VisitSyntaxDef(ast::SyntaxDef* p) -> bool {
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return true;
}

auto EffectVisitor::VisitVariableDef(ast::VariableDef* p) -> bool {
  ASSERT(p);
  ValueVisitor for_value(GetOwner());
  LOG_IF(ERROR, !p->GetVal()->Accept(&for_value)) << "failed to determine value for: " << p->ToString();
  Append(for_value);
  ReturnDefinition(new StoreVariableInstr(p->GetVar(), for_value.GetValue()));
  return true;
}

auto EffectVisitor::VisitConstantExpr(ast::ConstantExpr* p) -> bool {
  ASSERT(p);
  ReturnDefinition(new ConstantInstr(p->GetValue()));
  return true;
}

auto EffectVisitor::VisitQuoteExpr(ast::QuoteExpr* p) -> bool {
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return true;
}

auto EffectVisitor::VisitLoadVariableExpr(ast::LoadVariableExpr* p) -> bool {
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return true;
}

auto EffectVisitor::VisitBody(ast::Body* p) -> bool {
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return true;
}

auto EffectVisitor::VisitCallProcExpr(ast::CallProcExpr* expr) -> bool {
  ASSERT(expr);
  DLOG(INFO) << "symbol: " << expr->GetSymbol();
  DLOG(INFO) << "args: " << expr->GetArgs()->ToString();
  ValueVisitor for_args(GetOwner());
  if (!expr->GetArgs()->Accept(&for_args))
    return false;
  Append(for_args);
  ReturnDefinition(new CallProcInstr(expr->GetSymbol(), for_args.GetValue()));
  return true;
}

auto EffectVisitor::VisitExpressionList(ast::ExpressionList* expressions) -> bool {
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return true;
}

auto EffectVisitor::VisitBinaryOpExpr(ast::BinaryOpExpr* expr) -> bool {
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

  ReturnDefinition(new BinaryOpInstr(expr->GetOp()));
  return true;
}
}  // namespace scm