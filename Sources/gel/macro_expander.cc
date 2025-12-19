#include "macro_expander.h"

#include "common.h"
#include "expr/expression.h"
#include "expr/exprs.h"
#include "local.h"
#include "local_scope.h"
#include "macro.h"
#include "object.h"
#include "script.h"

namespace gel {
#define VISIT(Visitor, Expr)                                  \
  if (!Visitor((Expr))) {                                     \
    DLOG(ERROR) << "failed to visit: " << (Expr)->ToString(); \
    return false;                                             \
  }

auto MacroEffectVisitor::VisitExpressionList(const expr::ExpressionList& source, expr::ExpressionList& dest,
                                             bool* changed) -> bool {
  for (const auto& expr : source) {
    MacroEffectVisitor for_effect(GetOwner());
    if (!for_effect(expr)) {
      DLOG(ERROR) << "failed to visit: " << expr->ToString();
      (*changed) = false;
      return false;
    }
    if (!for_effect) {
      dest.push_back(expr);
      continue;
    }
    (*changed) = true;
    dest.insert(std::end(dest), std::begin(for_effect), std::end(for_effect));
  }
  return true;
}

auto MacroEffectVisitor::VisitBindingExpr(expr::BindingExpr* expr) -> bool {
  ASSERT(expr);
  MacroEffectVisitor for_value(GetOwner());
  VISIT(for_value, expr->GetValue());
  if (for_value)
    SetResult(expr::BindingExpr::New(expr->GetLocal(), for_value.GetResult()));
  return true;
}

auto MacroEffectVisitor::VisitForeachExpr(expr::ForeachExpr* expr) -> bool {
  ASSERT(expr);
  MacroEffectVisitor for_binding(GetOwner());
  VISIT(for_binding, expr->GetBindingAt(0));

  MacroEffectVisitor for_body(GetOwner());
  VISIT(for_body, expr->GetBody());

  if (for_binding || for_body) {
    const auto new_binding = for_binding ? for_binding.GetResult()->AsBindingExpr() : expr->GetBindingAt(0);
    const auto new_body = for_body ? for_body.GetResultAsSeq() : expr->GetBody();
    SetResult(expr::ForeachExpr::New(new_binding, new_body));
  }
  return true;
}

auto MacroEffectVisitor::VisitSeqExpr(expr::SeqExpr* expr) -> bool {
  ASSERT(expr);
  for (auto idx = 0; idx < expr->GetNumberOfChildren(); idx++) {
    do {
      const auto child = expr->GetChildAt(idx);
      ASSERT(child);
      MacroEffectVisitor for_effect(GetOwner());
      if (!child->Accept(for_effect) || !for_effect)
        break;
      expr->ReplaceChildAt(idx, for_effect.GetResults());
    } while (true);
  }
  return true;
}

auto MacroEffectVisitor::VisitDoExpr(expr::DoExpr* expr) -> bool {
  ASSERT(expr);
  MacroEffectVisitor for_body(GetOwner());
  VISIT(for_body, expr->GetBody());
  if (for_body)
    SetResult(expr::DoExpr::New(for_body.GetResultAsSeq()));
  return true;
}

auto MacroEffectVisitor::VisitUnaryOpExpr(expr::UnaryOpExpr* expr) -> bool {
  ASSERT(expr);
  MacroEffectVisitor for_value(GetOwner());
  VISIT(for_value, expr->GetValue());
  if (for_value) {
    ASSERT(for_value.GetNumberOfResults() == 1);
    SetResult(expr::UnaryOpExpr::New(expr->GetOp(), for_value.GetResult()));
  }
  return true;
}

auto MacroEffectVisitor::VisitBinaryOpExpr(expr::BinaryOpExpr* expr) -> bool {
  ASSERT(expr);
  MacroEffectVisitor for_left(GetOwner());
  VISIT(for_left, expr->GetLeft());
  MacroEffectVisitor for_right(GetOwner());
  VISIT(for_right, expr->GetRight());
  if (for_left || for_right) {
    const auto lhs = for_left ? for_left.GetResult() : expr->GetLeft();
    ASSERT(lhs);
    const auto rhs = for_right ? for_right.GetResult() : expr->GetRight();
    ASSERT(rhs);
    SetResult(expr::BinaryOpExpr::New(expr->GetOp(), lhs, rhs));
  }
  return true;
}

auto MacroEffectVisitor::VisitLoadFieldExpr(expr::LoadFieldExpr* expr) -> bool {
  ASSERT(expr);
  MacroEffectVisitor for_instance(GetOwner());
  VISIT(for_instance, expr->GetInstance());
  if (for_instance) {
    ASSERT(for_instance.GetNumberOfResults() == 1);
    SetResult(expr::LoadFieldExpr::New(for_instance.GetResult(), expr->GetField()));
  }
  return true;
}

auto MacroEffectVisitor::VisitInvokeMacroExpr(expr::InvokeMacroExpr* expr) -> bool {
  ASSERT(expr);
  const auto macro = expr->GetTarget();
  ASSERT(macro);
  bool changed = false;
  expr::ExpressionList body{};
  for (const auto& e : macro->GetBody()) {
    ASSERT(e);
    MacroExpansionSiteEffectVisitor for_effect(GetOwner(), expr->GetExpansionSite());
    VISIT(for_effect, e);
    if (for_effect) {
      changed = true;
      body.insert(std::end(body), std::begin(for_effect), std::end(for_effect));
      continue;
    }
    ASSERT(!for_effect.HasResult());
    body.push_back(e);
  }

  if (changed)
    SetResult(body);
  return true;
}

auto MacroEffectVisitor::VisitInvokeNativeExpr(expr::InvokeNativeExpr* expr) -> bool {
  bool changed = false;
  expr::ExpressionList new_args{};
  new_args.reserve(expr->GetNumberOfArgs());
  if (!VisitExpressionList(expr->GetArgs(), new_args, &changed))
    return false;
  if (changed)
    SetResult(expr::InvokeNativeExpr::New(expr->GetTarget(), new_args));
  return true;
}

auto MacroEffectVisitor::VisitInvokeInstanceExpr(expr::InvokeInstanceExpr* expr) -> bool {
  ASSERT(expr);
  MacroEffectVisitor for_instance(GetOwner());
  if (!for_instance(expr->GetInstance())) {
    LOG(ERROR) << "failed to visit: " << expr->GetInstance()->ToString();
    return false;
  }

  bool changed = false;
  expr::ExpressionList new_args{};
  new_args.reserve(expr->GetNumberOfArgs());
  if (!VisitExpressionList(expr->GetArgs(), new_args, &changed))
    return false;
  if (changed || for_instance) {
    const auto instance = for_instance ? for_instance.GetResult() : expr->GetInstance();
    SetResult(expr::InvokeInstanceExpr::New(expr->GetTarget(), instance, new_args));
  }
  return true;
}

auto MacroEffectVisitor::VisitInvokeExpr(expr::InvokeExpr* expr) -> bool {
  bool changed = false;
  expr::ExpressionList new_args{};
  new_args.reserve(expr->GetNumberOfArgs());
  if (!VisitExpressionList(expr->GetArgs(), new_args, &changed))
    return false;
  if (changed)
    SetResult(expr::InvokeExpr::New(expr->GetTarget(), new_args));
  return true;
}

auto MacroEffectVisitor::VisitClauseExpr(expr::ClauseExpr* expr) -> bool {
  ASSERT(expr);
  MacroEffectVisitor for_test(GetOwner());
  VISIT(for_test, expr->GetKey());
  MacroEffectVisitor for_body(GetOwner());
  VISIT(for_body, expr->GetBody());
  if (for_test || for_body)
    SetResult(expr::ClauseExpr::New(for_test, for_body));
  return true;
}

auto MacroEffectVisitor::VisitCondExpr(expr::CondExpr* expr) -> bool {
  ASSERT(expr);
  bool changed = false;
  expr::ClauseList clauses{};
  for (const auto& clause : expr->GetClauses()) {
    MacroEffectVisitor for_effect(GetOwner());
    VISIT(for_effect, clause);
    if (!for_effect) {
      clauses.push_back(clause);
      continue;
    }
    changed = true;
    clauses.push_back(for_effect.GetResult()->AsClauseExpr());
  }
  if (changed)
    SetResult(expr::CondExpr::New(clauses));
  return true;
}

auto MacroEffectVisitor::VisitImportExpr(expr::ImportExpr* expr) -> bool {
  ASSERT(expr);
  // do nothing
  return true;
}

auto MacroEffectVisitor::VisitLetExpr(expr::LetExpr* expr) -> bool {
  ASSERT(expr);
  NOT_IMPLEMENTED(ERROR);
  // bool bindings_changed = false;
  // expr::BindingList new_bindings{};
  // for (const auto& binding : expr->GetBindings()) {
  //   MacroEffectVisitor for_binding(GetOwner());
  //   VISIT(for_binding, binding);
  //   if (!for_binding) {
  //     new_bindings.push_back(binding);
  //     continue;
  //   }
  //   // this c-style cast is okay because expr::Binding is a subclass of expr::Expression
  //   bindings_changed = true;
  //   const auto& results = (const expr::BindingList&)for_binding.GetResults();  //
  //   NOLINT(cppcoreguidelines-pro-type-cstyle-cast) new_bindings.insert(std::begin(new_bindings), std::begin(results),
  //   std::end(results));
  // }

  // bool body_changed = false;
  // expr::ExpressionList new_body{};
  // if (!VisitExpressionList(expr->GetBody(), new_body, &body_changed))
  //   return false;

  // if (bindings_changed || body_changed) {
  //   const auto bindings = bindings_changed ? new_bindings : expr->GetBindings();
  //   const auto body = body_changed ? new_body : expr->GetBody();
  //   SetResult(expr::LetExpr::New(expr->GetScope(), bindings, body));
  // }
  return true;
}

auto MacroEffectVisitor::VisitLiteralExpr(expr::LiteralExpr* expr) -> bool {
  ASSERT(expr);
  if (!expr::IsLiteralSymbol(expr))
    return true;  // skip
  expr::ExpressionList expansion{};
  if (Expand(expr, expansion))
    SetResult(expansion);
  return true;
}

auto MacroEffectVisitor::VisitLoadInstanceMethodExpr(expr::LoadInstanceMethodExpr* expr) -> bool {
  ASSERT(expr);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return false;
}

auto MacroEffectVisitor::VisitNewExpr(expr::NewExpr* expr) -> bool {
  ASSERT(expr);
  bool args_changed = false;
  expr::ExpressionList new_args{};
  if (!VisitExpressionList(expr->GetArgs(), new_args, &args_changed))
    return false;
  if (args_changed)
    SetResult(expr::NewExpr::New(expr->GetTargetClass(), new_args));
  return true;
}

auto MacroEffectVisitor::VisitNewMapExpr(expr::NewMapExpr* expr) -> bool {
  ASSERT(expr);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return false;
}

auto MacroEffectVisitor::VisitStoreLocalExpr(expr::StoreLocalExpr* expr) -> bool {
  ASSERT(expr);
  MacroEffectVisitor for_value(GetOwner());
  VISIT(for_value, expr->GetValue());
  if (for_value)
    SetResult(for_value);
  return true;
}

auto MacroEffectVisitor::VisitStoreFieldExpr(expr::StoreFieldExpr* expr) -> bool {
  ASSERT(expr);
  MacroEffectVisitor for_instance(GetOwner());
  VISIT(for_instance, expr->GetInstance());

  MacroEffectVisitor for_value(GetOwner());
  VISIT(for_value, expr->GetValue());

  if (for_instance || for_value) {
    const auto instance = for_instance ? for_instance.GetResult() : expr->GetInstance();
    const auto value = for_value ? for_value.GetResult() : expr->GetValue();
    SetResult(expr::StoreFieldExpr::New(expr->GetField(), instance, value));
  }
  return true;
}

auto MacroEffectVisitor::VisitThrowExpr(expr::ThrowExpr* expr) -> bool {
  ASSERT(expr);
  MacroEffectVisitor for_value(GetOwner());
  VISIT(for_value, expr->GetValue());
  if (for_value)
    SetResult(expr::ThrowExpr::New(for_value.GetResult()));
  return true;
}

auto MacroEffectVisitor::VisitWhileExpr(expr::WhileExpr* expr) -> bool {
  ASSERT(expr);
  bool changed = false;

  MacroEffectVisitor for_test(GetOwner());
  if (for_test)
    changed = true;

  MacroEffectVisitor for_body(GetOwner());
  if (!expr->GetBody()->Accept(for_body)) {
    LOG(ERROR) << "failed to visit while-expr body.";
    return false;
  }
  if (for_body)
    changed = true;

  if (changed) {
    const auto test = for_test ? for_test.GetResult() : expr->GetTest();
    const auto body = changed ? SeqExpr::New(for_body.GetResults()) : expr->GetBody();
    SetResult(expr::WhileExpr::New(test, body));
  }
  return true;
}

auto MacroExpansionSiteEffectVisitor::Expand(expr::LiteralExpr* expr, expr::ExpressionList& results) -> bool {
  ASSERT(expr && expr->GetValue()->IsSymbol());
  const auto symbol = expr->GetValue()->AsSymbol();
  ASSERT(symbol);
  const auto argument = GetSite().GetTarget()->GetArg(symbol);
  if (!argument)
    return false;
  auto index = argument->GetIndex();
  expr::Expression* value = index <= GetSite().GetNumberOfArgs() ? GetSite().GetArgAt(index) : nullptr;
  LOG_IF(FATAL, !value && !argument->IsOptional()) << "failed to get argument: " << argument->ToString();
  results.push_back(value);
  if (!argument->IsVararg())
    return true;
  while (++index < GetSite().GetNumberOfArgs()) {
    results.push_back(GetSite().GetArgAt(index));
  }
  return true;
}

auto MacroExpansionSiteEffectVisitor::VisitExpressionList(const expr::ExpressionList& source,
                                                          expr::ExpressionList& dest, bool* changed) -> bool {
  for (const auto& expr : source) {
    MacroExpansionSiteEffectVisitor for_effect(GetOwner(), GetSite());
    if (!for_effect(expr)) {
      DLOG(ERROR) << "failed to visit: " << expr->ToString();
      (*changed) = false;
      return false;
    }
    if (!for_effect) {
      dest.push_back(expr);
      continue;
    }
    (*changed) = true;
    dest.insert(std::end(dest), std::begin(for_effect), std::end(for_effect));
  }
  return true;
}

auto MacroExpansionSiteEffectVisitor::VisitInvokeExpr(expr::InvokeExpr* expr) -> bool {
  ASSERT(expr);
  MacroExpansionSiteEffectVisitor for_target(GetOwner(), GetSite());
  VISIT(for_target, expr->GetTarget());

  bool changed = false;
  expr::ExpressionList new_args{};
  if (!VisitExpressionList(expr->GetArgs(), new_args, &changed))
    return false;
  if (for_target || changed) {
    const auto new_target = for_target ? for_target.GetResult() : expr->GetTarget();
    ASSERT(new_target);
    SetResult(expr::InvokeExpr::New(new_target, changed ? new_args : expr->GetArgs()));
  }
  return true;
}

auto MacroExpansionSiteEffectVisitor::VisitLiteralExpr(expr::LiteralExpr* expr) -> bool {
  ASSERT(expr);
  if (!expr->IsLiteralSymbol())
    return true;
  expr::ExpressionList results{};
  if (Expand(expr, results))
    SetResult(results);
  return true;
}
}  // namespace gel
