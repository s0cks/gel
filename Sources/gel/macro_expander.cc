#include "gel/macro_expander.h"

#include "gel/common.h"
#include "gel/expression.h"
#include "gel/local.h"
#include "gel/local_scope.h"
#include "gel/macro.h"
#include "gel/script.h"

namespace gel {
auto MacroExpander::ExpandAllInLambda(Lambda* lambda) -> bool {
  ASSERT(lambda);
  const auto scope = PushScope(lambda->HasScope() ? std::vector<LocalScope*>{lambda->GetScope()} : std::vector<LocalScope*>{});
  ASSERT(scope);
  for (auto idx = 0; idx < lambda->GetNumberOfExpressions(); idx++) {
    do {
      const auto expr = lambda->GetExpressionAt(idx);
      ASSERT(expr);
      MacroEffectVisitor for_effect(this);
      LOG_IF(ERROR, !expr->Accept(&for_effect)) << "failed to visit " << expr->ToString();
      if (!for_effect)
        break;
      lambda->ReplaceExpressionAt(idx, for_effect.GetResults());
    } while (true);
  }
  PopScope();
  return true;
}

auto MacroExpander::ExpandAllInScript(Script* script) -> bool {
  ASSERT(script);
  const auto scope = PushScope({script->GetScope()});
  ASSERT(scope);
  for (auto idx = 0; idx < script->GetNumberOfExpressions(); idx++) {
    do {
      const auto expr = script->GetExpressionAt(idx);
      ASSERT(expr);
      MacroEffectVisitor for_effect(this);
      LOG_IF(ERROR, !expr->Accept(&for_effect)) << "failed to visit " << expr->ToString();
      if (!for_effect)
        break;
      script->ReplaceExpressionAt(idx, for_effect.GetResults());
    } while (true);
  }
  PopScope();
  return true;
}

auto MacroEffectVisitor::VisitBeginExpr(expr::BeginExpr* expr) -> bool {
  ASSERT(expr);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return true;
}

auto MacroEffectVisitor::VisitBinaryOpExpr(expr::BinaryOpExpr* expr) -> bool {
  ASSERT(expr);
  // lhs
  MacroEffectVisitor for_left(GetOwner());
  LOG_IF(ERROR, !expr->GetLeft()->Accept(&for_left))
      << "failed to visit left expr " << expr->GetLeft()->ToString() << " of " << expr->ToString();
  // rhs
  MacroEffectVisitor for_right(GetOwner());
  LOG_IF(ERROR, !expr->GetRight()->Accept(&for_right))
      << "failed to visit right expr " << expr->GetRight()->ToString() << " of " << expr->ToString();
  if (for_left.HasResult() || for_right.HasResult()) {
    const auto lhs = for_left ? for_left.GetResult() : expr->GetLeft();
    const auto rhs = for_right ? for_right.GetResult() : expr->GetRight();
    SetResult(expr::BinaryOpExpr::New(expr->GetOp(), lhs, rhs));
    return true;
  }
  return false;
}

auto MacroEffectVisitor::VisitBinding(expr::Binding* expr) -> bool {
  ASSERT(expr);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return true;
}

static inline auto IsMacroCall(LocalScope* scope, expr::Expression* expr, Macro** macro) -> bool {
  ASSERT(scope);
  if (!expr || !expr->IsCallProcExpr()) {
    (*macro) = nullptr;
    return false;
  }
  const auto target = expr->AsCallProcExpr()->GetTarget();
  if (!expr::IsLiteralSymbol(target)) {
    (*macro) = nullptr;
    return false;
  }
  const auto symbol = target->AsLiteralExpr()->GetValue()->AsSymbol();
  ASSERT(symbol);
  LocalVariable* local = nullptr;
  if (!scope->Lookup(symbol, &local)) {
    (*macro) = nullptr;
    return false;
  }
  ASSERT(local);
  if (!local->HasValue() || !local->GetValue()->IsMacro()) {
    (*macro) = nullptr;
    return false;
  }
  (*macro) = local->GetValue()->AsMacro();
  return true;
}

auto MacroEffectVisitor::VisitCallProcExpr(expr::CallProcExpr* expr) -> bool {
  ASSERT(expr);
  Macro* macro = nullptr;
  if (!IsMacroCall(GetOwner()->GetScope(), expr, &macro)) {
    bool changed = false;
    expr::ExpressionList new_args{};
    new_args.resize(expr->GetNumberOfArgs());
    for (auto idx = 0; idx < expr->GetNumberOfArgs(); idx++) {
      const auto arg = expr->GetArgAt(idx);
      ASSERT(arg);
      MacroEffectVisitor for_effect(GetOwner());
      if (!arg->Accept(&for_effect) || !for_effect.HasResult()) {
        new_args[idx] = arg;
        continue;
      }
      changed = true;
      new_args[idx] = for_effect.GetResult();
    }
    if (changed)
      SetResult(expr::CallProcExpr::New(expr->GetTarget(), new_args));
    return true;
  }
  ASSERT(macro->GetNumberOfArgs() == expr->GetNumberOfArgs());

  const auto macro_scope = GetOwner()->PushScope();
  ASSERT(macro_scope);
  for (const auto& arg : macro->GetArgs()) {
    const auto value = expr->GetArgAt(arg.GetIndex());
    ASSERT(value);
    LocalVariable* local = LocalVariable::New(macro_scope, arg.GetName(), value);
    ASSERT(local);
    if (!macro_scope->Add(local)) {
      LOG(ERROR) << "failed to add " << local << " to scope:";
      LocalScopePrinter::Print<google::ERROR, false>(macro_scope, __FILE__, __LINE__);
      LOG(FATAL) << "";
      return false;
    }
  }

  expr::ExpressionList body{};
  for (const auto& e : macro->GetBody()) {
    ASSERT(e);
    MacroEffectVisitor for_effect(GetOwner());
    LOG_IF(ERROR, !e->Accept(&for_effect)) << "failed to visit " << e->ToString();
    if (!for_effect || !for_effect.HasResult()) {
      body.push_back(e);
      continue;
    }
    body.insert(std::end(body), for_effect.begin(), for_effect.end());
  }
  SetResult(body);
  GetOwner()->PopScope();
  return true;
}

auto MacroEffectVisitor::VisitCaseExpr(expr::CaseExpr* expr) -> bool {
  ASSERT(expr);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto MacroEffectVisitor::VisitCastExpr(expr::CastExpr* expr) -> bool {
  ASSERT(expr);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto MacroEffectVisitor::VisitClauseExpr(expr::ClauseExpr* expr) -> bool {
  ASSERT(expr);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto MacroEffectVisitor::VisitCondExpr(expr::CondExpr* expr) -> bool {
  ASSERT(expr);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return false;
}

auto MacroEffectVisitor::VisitImportExpr(expr::ImportExpr* expr) -> bool {
  ASSERT(expr);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return false;
}

auto MacroEffectVisitor::VisitInstanceOfExpr(expr::InstanceOfExpr* expr) -> bool {
  ASSERT(expr);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto MacroEffectVisitor::VisitLetExpr(expr::LetExpr* expr) -> bool {
  ASSERT(expr);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto MacroEffectVisitor::VisitLetRxExpr(expr::LetRxExpr* expr) -> bool {
  ASSERT(expr);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return false;
}

auto MacroEffectVisitor::VisitListExpr(expr::ListExpr* expr) -> bool {
  ASSERT(expr);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto MacroEffectVisitor::VisitLiteralExpr(expr::LiteralExpr* expr) -> bool {
  ASSERT(expr);
  if (!expr::IsLiteralSymbol(expr))
    return true;  // skip
  const auto symbol = expr->GetValue()->AsSymbol();
  ASSERT(symbol);
  const auto scope = GetOwner()->GetScope();
  ASSERT(scope);
  LocalVariable* local = nullptr;
  if (!scope->Lookup(symbol, &local, false))
    return true;  // skip
  if (!local || !local->HasValue() || !local->GetValue()->IsExpression())
    return true;  // skip
  SetResult(local->GetValue()->AsExpression());
  return true;
}

auto MacroEffectVisitor::VisitLoadInstanceMethodExpr(expr::LoadInstanceMethodExpr* expr) -> bool {
  ASSERT(expr);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto MacroEffectVisitor::VisitLocalDef(expr::LocalDef* expr) -> bool {
  ASSERT(expr);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto MacroEffectVisitor::VisitNewExpr(expr::NewExpr* expr) -> bool {
  ASSERT(expr);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return false;
}

auto MacroEffectVisitor::VisitNewMapExpr(expr::NewMapExpr* expr) -> bool {
  ASSERT(expr);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto MacroEffectVisitor::VisitQuotedExpr(expr::QuotedExpr* expr) -> bool {
  ASSERT(expr);
  return false;
}

auto MacroEffectVisitor::VisitRxOpExpr(expr::RxOpExpr* expr) -> bool {
  ASSERT(expr);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto MacroEffectVisitor::VisitSetExpr(expr::SetExpr* expr) -> bool {
  ASSERT(expr);
  const auto value = expr->GetValue();
  ASSERT(value);
  MacroEffectVisitor for_value(GetOwner());
  if (!value->Accept(&for_value) || !for_value)
    return false;  // skip
  if (for_value.HasResult())
    expr->SetValue(for_value.GetResult());
  return for_value.HasResult();
}

auto MacroEffectVisitor::VisitThrowExpr(expr::ThrowExpr* expr) -> bool {
  ASSERT(expr);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto MacroEffectVisitor::VisitUnaryExpr(expr::UnaryExpr* expr) -> bool {
  ASSERT(expr);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto MacroEffectVisitor::VisitWhenExpr(expr::WhenExpr* expr) -> bool {
  ASSERT(expr);
  MacroEffectVisitor for_test(GetOwner());
  LOG_IF(WARNING, !expr->GetTest()->Accept(&for_test)) << "failed to visit when test: " << expr->GetTest()->ToString();

  bool changed = false;
  expr::ExpressionList new_actions{};
  for (auto idx = 0; idx < expr->GetNumberOfActions(); idx++) {
    const auto action = expr->GetActionAt(idx);
    ASSERT(action);
    MacroEffectVisitor for_action(GetOwner());
    LOG_IF(ERROR, !action->Accept(&for_action)) << "failed to visit when action #" << idx << " " << action->ToString();
    if (!for_action) {
      new_actions.push_back(action);
      continue;
    }
    changed = true;
    new_actions.insert(std::end(new_actions), for_action.begin(), for_action.end());
  }
  if (for_test.HasResult() || changed) {
    const auto test = for_test ? for_test.GetResult() : expr->GetTest();
    const auto& actions = changed ? new_actions : expr->GetActions();
    SetResult(expr::WhenExpr::New(test, actions));
  }
  return true;
}

auto MacroEffectVisitor::VisitWhileExpr(expr::WhileExpr* expr) -> bool {
  ASSERT(expr);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}
}  // namespace gel