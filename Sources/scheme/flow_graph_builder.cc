#include "scheme/flow_graph_builder.h"

#include <glog/logging.h>

#include "scheme/common.h"
#include "scheme/expression.h"
#include "scheme/instruction.h"
#include "scheme/lambda.h"
#include "scheme/local.h"
#include "scheme/native_procedure.h"
#include "scheme/object.h"
#include "scheme/procedure.h"
#include "scheme/script.h"

namespace scm {
static inline auto AppendFragment(EntryInstr* entry, EffectVisitor& vis) -> Instruction* {
  ASSERT(entry);
  if (vis.IsEmpty())
    return entry;
  entry->Append(vis.GetEntryInstr());
  return vis.GetExitInstr();
}

auto EffectVisitor::VisitEvalExpr(EvalExpr* expr) -> bool {
  ASSERT(expr && expr->HasExpression());
  ValueVisitor for_value(GetOwner());
  if (!expr->GetExpression()->Accept(&for_value)) {
    LOG(ERROR) << "failed to visit EvalExpr expr: " << expr->GetExpression()->ToString();
    return false;
  }
  Append(for_value);
  ReturnDefinition(EvalInstr::New(for_value.GetValue()));
  return true;
}

static inline auto IsNativeCall(instr::Instruction* instr) -> bool {
  ASSERT(instr);
  if (!instr->IsConstantInstr())
    return false;
  const auto target = instr->AsConstantInstr()->GetValue();
  ASSERT(target);
  return target->IsNativeProcedure();
}

// static inline auto IsMacroCall(instr::Instruction* instr) -> bool {
//   ASSERT(instr);
//   if (!instr->IsConstantInstr())
//     return false;
//   const auto target = instr->AsConstantInstr()->GetValue();
//   ASSERT(target);
//   return target->IsMacro();
// }

auto EffectVisitor::VisitCallProcExpr(CallProcExpr* expr) -> bool {
  ASSERT(expr);
  ValueVisitor for_target(GetOwner());
  {
    const auto target = expr->GetTarget();
    ASSERT(target);
    if (!target->Accept(&for_target)) {
      LOG(ERROR) << "failed to visit target: " << expr->GetTarget()->ToString();
      return false;
    }
  }
  const auto target = for_target.GetValue();
  ASSERT(target);
  for (auto idx = 1; idx < expr->GetNumberOfChildren(); idx++) {
    const auto arg = expr->GetChildAt(idx);
    ASSERT(arg);
    ValueVisitor for_value(GetOwner());
    LOG_IF(ERROR, !arg->Accept(&for_value)) << "failed to determine value for: " << expr->ToString();
    Append(for_value);
  }
  Append(for_target);

  if (IsNativeCall(target)) {
    Add(InstanceOfInstr::New(target, NativeProcedure::GetClass()));
    ReturnDefinition(InvokeNativeInstr::New(for_target.GetValue(), expr->GetNumberOfArgs()));
    return true;
  }

  Add(InstanceOfInstr::New(target, Procedure::GetClass()));
  ReturnDefinition(InvokeInstr::New(target, expr->GetNumberOfArgs()));
  return true;
}

auto EffectVisitor::VisitCaseExpr(expr::CaseExpr* expr) -> bool {
  ASSERT(expr);
  const auto join = JoinEntryInstr::New(GetOwner()->GetNextBlockId());
  ASSERT(join);

  for (const auto& clause : expr->GetClauses()) {
    ASSERT(clause);
    EffectVisitor for_clause(GetOwner());
    if (clause && !clause->Accept(&for_clause)) {
      LOG(ERROR) << "failed to visit clause: " << clause->ToString();
      return false;
    }
    for_clause.Add(GotoInstr::New(join));

    ValueVisitor for_test(GetOwner());
    if (!expr->GetKey()->Accept(&for_test)) {
      return false;
    }
    ASSERT(clause->GetKey());
    if (!clause->GetKey()->Accept(&for_test)) {
      LOG(ERROR) << "failed to visit test for cond: " << expr->ToString();
      return false;
    }

    ASSERT(for_clause.GetEntryInstr() != nullptr && for_clause.GetEntryInstr()->IsEntryInstr());
    const auto target = for_clause.GetEntryInstr()->AsEntryInstr();
    const auto cmp = instr::BinaryOpInstr::NewEquals(for_test.GetValue(), for_test.GetValue());  // TODO: fix this
    for_test.Add(cmp);
    const auto branch = BranchInstr::New(cmp, target, join);
    for_test.Add(branch);
    Append(for_test);
    GetOwner()->GetCurrentBlock()->AddDominated(target);
  }

  SetExitInstr(join);
  GetOwner()->GetCurrentBlock()->AddDominated(join);
  return true;
}

auto EffectVisitor::VisitClauseExpr(expr::ClauseExpr* expr) -> bool {
  ASSERT(expr);

  const auto target = TargetEntryInstr::New(GetOwner()->GetNextBlockId());
  ASSERT(target);
  Add(target);

  auto remaining = expr->GetNumberOfActions();
  for (const auto& action : expr->GetActions()) {
    ASSERT(action);
    EffectVisitor for_action(GetOwner());
    if (!action->Accept(&for_action)) {
      LOG(ERROR) << "failed to visit action for: " << expr->ToString();
      return false;
    }
    if (--remaining <= 0)
      for_action.AddImplicitReturn();
    AppendFragment(target, for_action);
    SetExitInstr(for_action.GetExitInstr());
  }
  GetOwner()->GetCurrentBlock()->AddDominated(target);
  return true;
}

auto EffectVisitor::VisitWhenExpr(expr::WhenExpr* expr) -> bool {
  ASSERT(expr);
  const auto join = JoinEntryInstr::New(GetOwner()->GetNextBlockId());
  ASSERT(join);

  // process conseq
  const auto conseq_target = TargetEntryInstr::New(GetOwner()->GetNextBlockId());
  for (const auto& action : expr->GetActions()) {
    EffectVisitor for_conseq(GetOwner());
    if (!action->Accept(&for_conseq)) {
      LOG(ERROR) << "failed to visit action for: " << expr->ToString();
      return false;
    }
    AppendFragment(conseq_target, for_conseq);
  }
  conseq_target->Append(GotoInstr::New(join));
  GetOwner()->GetCurrentBlock()->AddDominated(conseq_target);

  // process test
  ValueVisitor for_test(GetOwner());
  if (!expr->GetTest()->Accept(&for_test)) {
    LOG(ERROR) << "failed to visit test for cond: " << expr->ToString();
    return false;
  }
  Append(for_test);

  const auto branch = BranchInstr::New(for_test.GetValue(), conseq_target, join);
  ASSERT(branch);
  Add(branch);
  SetExitInstr(join);
  GetOwner()->GetCurrentBlock()->AddDominated(join);
  return true;
}

auto EffectVisitor::VisitMacroDef(MacroDef* expr) -> bool {
  ASSERT(expr);
  // TODO:
  //  MacroCompiler compiler;
  //  const auto macro = compiler.CompileMacro(expr);
  //  ASSERT(macro);
  // const auto scope = GetOwner()->GetScope();
  // ASSERT(scope);
  // if (!scope->Add(macro->GetSymbol(), macro)) {
  //   throw Exception(
  //       fmt::format("cannot define Macro symbol `{}` for value: `{}`", macro->GetSymbol()->ToString(), macro->ToString()));
  // }
  return true;
}

auto EffectVisitor::VisitWhileExpr(expr::WhileExpr* expr) -> bool {  // TODO: clean this up
  ASSERT(expr);
  const auto target = TargetEntryInstr::New(GetOwner()->GetNextBlockId());
  ASSERT(target);
  Add(target);

  const auto body_target = TargetEntryInstr::New(GetOwner()->GetNextBlockId());
  ASSERT(body_target);

  const auto join = JoinEntryInstr::New(GetOwner()->GetNextBlockId());
  ASSERT(join);

  ValueVisitor for_test(GetOwner());
  if (!expr->GetTest()->Accept(&for_test)) {
    LOG(ERROR) << "failed to visit test for: " << expr->ToString();
    return false;
  }
  AppendFragment(target, for_test);
  target->Append(BranchInstr::New(for_test.GetValue(), body_target, join));

  EffectVisitor for_body(GetOwner());
  for (const auto& expr : expr->GetBody()) {
    if (!expr->Accept(&for_body)) {
      LOG(ERROR) << "failed to visit action for: " << expr->ToString();
      return false;
    }
  }
  AppendFragment(body_target, for_body);
  body_target->Append(instr::GotoInstr::New(target));

  SetExitInstr(join);
  GetOwner()->GetCurrentBlock()->AddDominated(target);
  GetOwner()->GetCurrentBlock()->AddDominated(join);
  return true;
}

auto EffectVisitor::VisitImportDef(expr::ImportDef* expr) -> bool {
  ASSERT(expr);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto EffectVisitor::VisitQuotedExpr(expr::QuotedExpr* expr) -> bool {
  ASSERT(expr);
  ReturnDefinition(ConstantInstr::New(expr->Get()));
  return true;
}

auto EffectVisitor::VisitLetExpr(expr::LetExpr* expr) -> bool {
  ASSERT(expr);
  const auto target = TargetEntryInstr::New(GetOwner()->GetNextBlockId());
  ASSERT(target);
  Add(target);

  {
    // process bindings first
    uint64_t idx = 0;
    while (IsOpen() && (idx < expr->GetNumberOfBindings())) {
      const auto& binding = expr->GetBindingAt(idx++);
      ValueVisitor for_value(GetOwner());
      if (!binding.GetValue()->Accept(&for_value)) {
        LOG(ERROR) << "failed to visit: " << binding;
        return false;
      }
      Append(for_value);
      Add(instr::StoreVariableInstr::New(binding.GetSymbol(), for_value.GetValue()));
    }
  }

  {
    // process body
    uint64_t idx = 0;
    while (IsOpen() && (idx < expr->GetNumberOfChildren())) {
      const auto child = expr->GetChildAt(idx++);
      ASSERT(child);
      EffectVisitor for_effect(GetOwner());
      if (!child->Accept(&for_effect))
        break;
      Append(for_effect);
      if (!IsOpen())
        break;
    }
  }
  return true;
}

auto EffectVisitor::VisitBeginExpr(BeginExpr* expr) -> bool {
  const auto target = TargetEntryInstr::New(GetOwner()->GetNextBlockId());
  ASSERT(target);
  Add(target);

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

  AddImplicitReturn();
  return true;
}

auto EffectVisitor::VisitCondExpr(CondExpr* expr) -> bool {
  ASSERT(expr);
  const auto join = JoinEntryInstr::New(GetOwner()->GetNextBlockId());

  for (const auto& clause : expr->GetClauses()) {
    // process conseq
    const auto target = TargetEntryInstr::New(GetOwner()->GetNextBlockId());
    ASSERT(target);
    for (const auto& action : clause->GetActions()) {
      ASSERT(action);
      ValueVisitor for_action(GetOwner());
      if (!action->Accept(&for_action)) {
        LOG(ERROR) << "failed to visit conseq for cond: " << expr->ToString();
        return false;
      }
      AppendFragment(target, for_action);
    }
    target->Append(GotoInstr::New(join));
    GetOwner()->GetCurrentBlock()->AddDominated(target);

    ValueVisitor for_test(GetOwner());
    if (!clause->GetKey()->Accept(&for_test)) {
      LOG(ERROR) << "failed to visit clause for cond: " << expr->ToString();
      return false;
    }
    Append(for_test);
    const auto branch = BranchInstr::New(for_test.GetValue(), target, join);
    ASSERT(branch);
    Add(branch);
  }

  if (expr->HasAlternate()) {  // process alt (else)
    const auto target = TargetEntryInstr::New(GetOwner()->GetNextBlockId());
    ASSERT(target);
    Add(target);
    ValueVisitor for_alt(GetOwner());
    if (!expr->GetAlternate()->Accept(&for_alt)) {
      LOG(ERROR) << "failed to visit alternate for cond: " << expr->ToString();
      return false;
    }
    AppendFragment(target, for_alt);
    target->Append(GotoInstr::New(join));
    GetOwner()->GetCurrentBlock()->AddDominated(target);
  }

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
  switch (expr->GetOp()) {
    case expr::kCar:
    case expr::kCdr:
      Add(instr::InstanceOfInstr::New(for_value.GetValue(), Pair::GetClass()));
    default:
      ReturnDefinition(instr::UnaryOpInstr::New(expr->GetOp(), for_value.GetValue()));
  }
  return true;
}

auto EffectVisitor::VisitLocalDef(LocalDef* expr) -> bool {
  ASSERT(expr);
  const auto symbol = expr->GetSymbol();
  ASSERT(symbol);
  // process value
  const auto value = expr->GetValue();
  ASSERT(value);
  ValueVisitor for_value(GetOwner());
  if (!value->Accept(&for_value)) {
    LOG(FATAL) << "failed to determine value for: " << expr->ToString();
    return false;
  }
  Append(for_value);
  Add(StoreVariableInstr::New(symbol, for_value.GetValue()));
  return true;
}

auto EffectVisitor::VisitLiteralExpr(LiteralExpr* p) -> bool {
  ASSERT(p);
  const auto value = p->GetValue();
  ASSERT(value);
  if (value->IsSymbol()) {
    LocalVariable* local = nullptr;
    if (!GetOwner()->GetScope()->Lookup(value->AsSymbol(), &local)) {
      ReturnDefinition(instr::LoadVariableInstr::New(value->AsSymbol()));
      return true;
    }
    ASSERT(local);
    if (local->HasValue() && local->GetValue()->IsNativeProcedure()) {
      ReturnDefinition(ConstantInstr::New(local->GetValue()));
      return true;
    }
    ReturnDefinition(instr::LoadVariableInstr::New(value->AsSymbol()));
    return true;
  } else {
    ReturnDefinition(ConstantInstr::New(p->GetValue()));
  }
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

  ReturnDefinition(BinaryOpInstr::New(expr->GetOp(), for_left.GetValue(), for_right.GetValue()));
  return true;
}

auto EffectVisitor::VisitThrowExpr(expr::ThrowExpr* expr) -> bool {
  ASSERT(expr && expr->HasValue());
  ValueVisitor for_value(GetOwner());
  if (!expr->GetValue()->Accept(&for_value)) {
    LOG(FATAL) << "failed to visit value: " << expr->GetValue()->ToString();
    return false;
  }
  Append(for_value);
  Add(InstanceOfInstr::New(for_value.GetValue(), String::GetClass()));
  Add(ThrowInstr::New(for_value.GetValue()));
  return true;
}

auto EffectVisitor::VisitSetExpr(expr::SetExpr* expr) -> bool {
  ASSERT(expr && expr->HasValue());
  const auto symbol = expr->GetSymbol();
  ASSERT(symbol);
  ValueVisitor for_value(GetOwner());
  if (!expr->GetValue()->Accept(&for_value)) {
    LOG(FATAL) << "failed to visit SetExpr value: " << expr->GetValue()->ToString();
    return false;
  }
  Append(for_value);
  Add(StoreVariableInstr::New(expr->GetSymbol(), for_value.GetValue()));
  return true;
}

auto FlowGraphBuilder::Build(Expression* expr, LocalScope* scope) -> FlowGraph* {
  ASSERT(expr);
  ASSERT(scope);
  FlowGraphBuilder builder(scope);
  const auto graph_entry = GraphEntryInstr::New(builder.GetNextBlockId());
  ASSERT(graph_entry);
  builder.SetCurrentBlock(graph_entry);
  const auto target = TargetEntryInstr::New(builder.GetNextBlockId());
  ASSERT(target);
  builder.SetCurrentBlock(target);
  EffectVisitor for_effect(&builder);
  if (!expr->Accept(&for_effect)) {
    LOG(ERROR) << "failed to visit: " << expr->ToString();
    return nullptr;  // TODO: free entry
  }
  const auto exit = for_effect.GetExitInstr();
  ASSERT(exit);
  if (exit && !exit->IsReturnInstr()) {
    const auto new_exit = exit->IsDefinition() ? instr::ReturnInstr::New(exit->AsDefinition()) : instr::ReturnInstr::New();
    ASSERT(new_exit);
    for_effect.Add(new_exit);
  }
  ASSERT(for_effect.GetExitInstr() && for_effect.GetExitInstr()->IsReturnInstr());
  AppendFragment(target, for_effect);
  graph_entry->Append(target);
  graph_entry->AddDominated(target);
  return new FlowGraph(graph_entry);
}

auto FlowGraphBuilder::Build(Script* script, LocalScope* scope) -> FlowGraph* {
  ASSERT(script);
  ASSERT(scope);
  FlowGraphBuilder builder(scope);
  const auto graph_entry = GraphEntryInstr::New(builder.GetNextBlockId());
  ASSERT(graph_entry);
  builder.SetCurrentBlock(graph_entry);
  const auto target = TargetEntryInstr::New(builder.GetNextBlockId());
  ASSERT(target);
  builder.SetCurrentBlock(target);
  const auto& body = script->GetBody();
  auto remaining = body.size();
  for (const auto& expr : body) {
    EffectVisitor for_effect(&builder);
    if (!expr->Accept(&for_effect)) {
      LOG(ERROR) << "failed to visit: " << expr->ToString();
      return nullptr;  // TODO: free entry
    }
    if (--remaining == 0) {
      const auto exit = for_effect.GetExitInstr();
      if (exit && !exit->IsReturnInstr())
        for_effect.Add(exit->IsDefinition() ? instr::ReturnInstr::New(exit->AsDefinition()) : instr::ReturnInstr::New());
    }
    AppendFragment(target, for_effect);
  }
  graph_entry->Append(target);
  graph_entry->AddDominated(target);
  return new FlowGraph(graph_entry);
}
}  // namespace scm