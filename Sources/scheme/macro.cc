#include "scheme/macro.h"

#include <sstream>

#include "scheme/common.h"
#include "scheme/expression.h"
#include "scheme/local.h"

namespace scm {
auto Macro::Equals(Type* rhs) const -> bool {
  if (!rhs || !rhs->IsMacro())
    return false;
  const auto other = rhs->AsMacro();
  ASSERT(other);
  if ((HasBody() && !other->HasBody()) || (!HasBody() && other->HasBody()))
    return false;
  return GetSymbol()->Equals(other->GetSymbol());
}

auto Macro::ToString() const -> std::string {
  std::stringstream ss;
  ss << "Macro(";
  ss << "symbol=" << GetSymbol();
  ss << ")";
  return ss.str();
}

class MacroEvaluator : public expr::ExpressionVisitor {
  DEFINE_NON_COPYABLE_TYPE(MacroEvaluator);

 private:
  LocalScope* scope_;
  expr::Expression* result_ = nullptr;

 public:
  explicit MacroEvaluator(LocalScope* scope = LocalScope::New()) :
    scope_(scope) {}
  ~MacroEvaluator() override = default;

  auto GetScope() const -> LocalScope* {
    return scope_;
  }

  auto GetResult() const -> expr::Expression* {
    return result_;
  }

  inline auto HasResult() const -> bool {
    return GetResult() != nullptr;
  }

  auto Eval(expr::Expression* expr) -> bool {
    ASSERT(expr);
    if (!expr->Accept(this)) {
      LOG(FATAL) << "failed to eval: " << expr->ToString();
      return false;
    }
    return true;
  }

  auto VisitBeginExpr(expr::BeginExpr* expr) -> bool override {
    ASSERT(expr);
    expr::ExpressionList exprs;
    for (auto idx = 0; idx < expr->GetNumberOfChildren(); idx++) {
      const auto child = expr->GetChildAt(idx);
      ASSERT(child);

      MacroEvaluator eval(GetScope());
      if (!child->Accept(&eval)) {
        LOG(FATAL) << "failed to visit: " << child->ToString();
        return false;
      }

      if (eval.HasResult()) {
        const auto result = eval.GetResult();
        ASSERT(result);
        exprs.push_back(result);
      } else if (!child->IsMacroDef()) {
        exprs.push_back(child);
      }
    }
    result_ = expr::BeginExpr::New(exprs);
    return true;
  }

  auto VisitConsExpr(expr::ConsExpr* expr) -> bool override {
    ASSERT(expr);
    {
      // car
      const auto value = expr->GetCar();
      ASSERT(value);
      MacroEvaluator eval;
      if (!value->Accept(&eval)) {
        LOG(FATAL) << "failed to visit: " << value->ToString();
        return false;
      }
      if (eval.HasResult())
        expr->SetCar(eval.GetResult());
    }
    {
      // cdr
      const auto value = expr->GetCdr();
      ASSERT(value);
      MacroEvaluator eval;
      if (!value->Accept(&eval)) {
        LOG(FATAL) << "failed to visit: " << value->ToString();
        return false;
      }
      if (eval.HasResult())
        expr->SetCdr(eval.GetResult());
    }
    return true;
  }

  auto VisitLambdaExpr(expr::LambdaExpr* expr) -> bool override {
    ASSERT(expr);
    NOT_IMPLEMENTED(ERROR);  // TODO: implement
    return true;
  }

  auto VisitLiteralExpr(expr::LiteralExpr* expr) -> bool override {
    ASSERT(expr);
    NOT_IMPLEMENTED(ERROR);  // TODO: implement
    return true;
  }

  auto VisitUnaryExpr(expr::UnaryExpr* expr) -> bool override {
    ASSERT(expr);
    const auto value = expr->GetValue();
    ASSERT(value);
    MacroEvaluator eval;
    if (!value->Accept(&eval)) {
      LOG(FATAL) << "failed to visit: " << value->ToString();
      return false;
    }
    if (eval.HasResult())
      expr->SetValue(eval.GetResult());
    return true;
  }

  auto VisitBinaryOpExpr(expr::BinaryOpExpr* expr) -> bool override {
    ASSERT(expr);
    {
      // left
      const auto value = expr->GetLeft();
      ASSERT(value);
      MacroEvaluator eval;
      if (!value->Accept(&eval)) {
        LOG(FATAL) << "failed to visit: " << value->ToString();
        return false;
      }
      if (eval.HasResult())
        expr->SetLeft(eval.GetResult());
    }
    {
      // right
      const auto value = expr->GetRight();
      ASSERT(value);
      MacroEvaluator eval;
      if (!value->Accept(&eval)) {
        LOG(FATAL) << "failed to visit: " << value->ToString();
        return false;
      }
      if (eval.HasResult())
        expr->SetRight(eval.GetResult());
    }
    return true;
  }

  auto VisitCondExpr(expr::CondExpr* expr) -> bool override {
    ASSERT(expr);
    NOT_IMPLEMENTED(ERROR);  // TODO: implement
    return true;
  }

  auto VisitQuotedExpr(expr::QuotedExpr* expr) -> bool override {
    ASSERT(expr);
    NOT_IMPLEMENTED(ERROR);  // TODO: implement
    return true;
  }

  auto VisitLocalDef(expr::LocalDef* expr) -> bool override {
    ASSERT(expr);
    NOT_IMPLEMENTED(ERROR);  // TODO: implement
    return true;
  }

  auto VisitModuleDef(expr::ModuleDef* expr) -> bool override {
    ASSERT(expr);
    NOT_IMPLEMENTED(ERROR);  // TODO: implement
    return true;
  }

  auto VisitImportDef(expr::ImportDef* expr) -> bool override {
    ASSERT(expr);
    NOT_IMPLEMENTED(ERROR);  // TODO: implement
    return true;
  }

  auto VisitMacroDef(expr::MacroDef* expr) -> bool override {
    ASSERT(expr);

    const auto symbol = expr->GetSymbol();
    ASSERT(symbol);

    LocalVariable* local = nullptr;
    if (GetScope()->Lookup(symbol, &local, false)) {
      LOG(FATAL) << "cannot redefine macro: " << symbol;
      return false;
    }

    const auto macro = Macro::New(expr->GetSymbol(), expr->GetBody());
    ASSERT(macro);
    local = LocalVariable::New(GetScope(), symbol, macro);
    ASSERT(local);
    LOG_IF(FATAL, !GetScope()->Add(local)) << "failed to add local: " << (*local);
    return true;
  }

  auto VisitEvalExpr(expr::EvalExpr* expr) -> bool override {
    ASSERT(expr);
    NOT_IMPLEMENTED(ERROR);  // TODO: implement
    return true;
  }

  auto VisitCallProcExpr(expr::CallProcExpr* expr) -> bool override {
    ASSERT(expr);
    const auto target = expr->GetTarget();
    ASSERT(target);

    if (!target->IsConstantExpr()) {
      LOG(WARNING) << "cannot eval non-constant callproc target: " << target->ToString();
      return true;
    }

    const auto target_value = target->EvalToConstant();
    ASSERT(target_value && target_value->IsSymbol());

    LocalVariable* local = nullptr;
    if (!GetScope()->Lookup(target_value->AsSymbol(), &local)) {
      LOG(WARNING) << "cannot find macro: " << target_value;
      return true;
    }
    ASSERT(local);
    if (!local->HasValue()) {
      LOG(WARNING) << "cannot find macro: " << target_value;
      return true;
    }
    if (!local->GetValue()->IsMacro()) {
      LOG(WARNING) << target_value << " is not a macro.";
      return true;
    }

    const auto macro = local->GetValue()->AsMacro();
    ASSERT(macro);
    result_ = macro->GetBody();
    DLOG(INFO) << "expanded macro " << macro << " to: " << macro->GetBody()->ToString();
    return true;
  }

  auto VisitSetExpr(expr::SetExpr* expr) -> bool override {
    ASSERT(expr);
    NOT_IMPLEMENTED(ERROR);  // TODO: implement
    return true;
  }

  auto VisitThrowExpr(expr::ThrowExpr* expr) -> bool override {
    ASSERT(expr);
    NOT_IMPLEMENTED(ERROR);  // TODO: implement
    return true;
  }
};

auto MacroExpander::Expand(expr::Expression** expr) -> bool {
  ASSERT((*expr));
  MacroEvaluator eval;
  if (!(*expr)->Accept(&eval)) {
    LOG(ERROR) << "failed to expand: " << (*expr)->ToString();
    return false;
  }

  if (!eval.HasResult())
    return true;
  (*expr) = eval.GetResult();
  return true;
}
}  // namespace scm