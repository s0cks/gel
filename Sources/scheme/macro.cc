#include "scheme/macro.h"

#include <sstream>

#include "scheme/common.h"
#include "scheme/expression.h"
#include "scheme/local.h"
#include "scheme/local_scope.h"
#include "scheme/type.h"

namespace scm {
auto Macro::New(const ObjectList& args) -> Macro* {
  NOT_IMPLEMENTED(FATAL);
}

auto Macro::CreateClass() -> Class* {
  return Class::New(Object::GetClass(), "Macro");
}

auto Macro::Equals(Object* rhs) const -> bool {
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
  if (HasBody())
    ss << ", body=" << GetBody()->ToString();
  ss << ")";
  return ss.str();
}

// class MacroReplacer : public expr::ExpressionVisitor {
//   DEFINE_NON_COPYABLE_TYPE(MacroReplacer);

//  private:
//   Expression* result_ = nullptr;
//   MacroExpander* owner_;
//   LocalScope* scope_;

//   inline void SetResult(Expression* expr) {
//     ASSERT(expr);
//     result_ = expr;
//   }

//  public:
//   explicit MacroReplacer(MacroExpander* owner, LocalScope* scope) :
//     expr::ExpressionVisitor(),
//     owner_(owner),
//     scope_(scope) {
//     ASSERT(owner_);
//   }
//   ~MacroReplacer() override = default;

//   auto GetResult() const -> Expression* {
//     return result_;
//   }

//   auto GetScope() const -> LocalScope* {
//     return scope_;
//   }

//   inline auto HasResult() const -> bool {
//     return GetResult() != nullptr;
//   }

//   auto GetOwner() const -> MacroExpander* {
//     return owner_;
//   }

//   auto VisitBeginExpr(expr::BeginExpr* expr) -> bool override {
//     ASSERT(expr);
//     NOT_IMPLEMENTED(ERROR);  // TODO: implement
//     return true;
//   }

//   auto VisitClauseExpr(expr::ClauseExpr* expr) -> bool override {
//     ASSERT(expr);
//     NOT_IMPLEMENTED(ERROR);  // TODO: implement
//     return true;
//   }

//   auto VisitWhileExpr(expr::WhileExpr* expr) -> bool override {
//     ASSERT(expr);
//     NOT_IMPLEMENTED(ERROR);  // TODO: implement
//     return true;
//   }

//   auto VisitConsExpr(expr::ConsExpr* expr) -> bool override {
//     ASSERT(expr);
//     NOT_IMPLEMENTED(ERROR);  // TODO: implement
//     return true;
//   }

//   auto VisitLambdaExpr(expr::LambdaExpr* expr) -> bool override {
//     ASSERT(expr);
//     NOT_IMPLEMENTED(ERROR);  // TODO: implement
//     return true;
//   }

//   static inline auto GetConstantSymbol(expr::LiteralExpr* expr) -> std::optional<Symbol*> {
//     if (!expr || !expr->IsConstantExpr())
//       return std::nullopt;
//     const auto value = expr->EvalToConstant();
//     if (!value->IsSymbol())
//       return std::nullopt;
//     return {value->AsSymbol()};
//   }

//   static inline auto IsMacroArg(LocalScope* scope, Symbol* symbol) -> bool {
//     ASSERT(scope);
//     ASSERT(symbol);
//     LocalVariable* local = nullptr;
//     if (!scope->Lookup(symbol, &local))
//       return false;
//     ASSERT(local);
//     return local->HasValue();
//   }

//   auto VisitLiteralExpr(expr::LiteralExpr* expr) -> bool override {
//     ASSERT(expr);
//     const auto symbol = GetConstantSymbol(expr);
//     if (!symbol)
//       return true;
//     DLOG_IF(INFO, IsMacroArg(GetScope(), (*symbol))) << "resolving arg: " << (*symbol)->ToString();
//     return true;
//   }

//   auto VisitWhenExpr(expr::WhenExpr* expr) -> bool override {
//     ASSERT(expr);
//     NOT_IMPLEMENTED(ERROR);  // TODO: implement
//     return true;
//   }

//   auto VisitCaseExpr(expr::CaseExpr* expr) -> bool override {
//     ASSERT(expr);
//     NOT_IMPLEMENTED(ERROR);  // TODO: implement
//     return true;
//   }

//   auto VisitUnaryExpr(expr::UnaryExpr* expr) -> bool override {
//     ASSERT(expr);
//     NOT_IMPLEMENTED(ERROR);  // TODO: implement
//     return true;
//   }

//   auto VisitCondExpr(expr::CondExpr* expr) -> bool override {
//     ASSERT(expr);
//     {
//       MacroReplacer for_test(GetOwner());
//       if (!expr->GetTest()->Accept(&for_test))
//         return false;
//       if (for_test.HasResult()) {
//         DLOG(INFO) << "replaced cond test w/: " << for_test.GetResult()->ToString();
//         expr->SetTest(for_test.GetResult());
//       }
//     }

//     {
//       MacroReplacer for_conseq(GetOwner());
//       if (!expr->GetConseq()->Accept(&for_conseq))
//         return false;
//       if (for_conseq.HasResult()) {
//         DLOG(INFO) << "replaced cond conseq w/: " << for_conseq.GetResult()->ToString();
//         expr->SetConseq(for_conseq.GetResult());
//       }
//     }

//     if (expr->HasAlternate()) {
//       MacroReplacer for_alt(GetOwner());
//       if (!expr->GetAlternate()->Accept(&for_alt))
//         return false;
//       if (for_alt.HasResult()) {
//         DLOG(INFO) << "replaced cond alt w/: " << for_alt.GetResult()->ToString();
//         expr->SetAlt(for_alt.GetResult());
//       }
//     }
//     return true;
//   }

//   auto VisitQuotedExpr(expr::QuotedExpr* expr) -> bool override {
//     ASSERT(expr);
//     NOT_IMPLEMENTED(ERROR);  // TODO: implement
//     return true;
//   }

//   auto VisitLocalDef(expr::LocalDef* expr) -> bool override {
//     ASSERT(expr);
//     NOT_IMPLEMENTED(ERROR);  // TODO: implement
//     return true;
//   }

//   auto VisitBinaryOpExpr(expr::BinaryOpExpr* expr) -> bool override {
//     ASSERT(expr);
//     NOT_IMPLEMENTED(ERROR);  // TODO: implement
//     return true;
//   }

//   auto VisitModuleDef(expr::ModuleDef* expr) -> bool override {
//     ASSERT(expr);
//     NOT_IMPLEMENTED(ERROR);  // TODO: implement
//     return true;
//   }

//   auto VisitImportDef(expr::ImportDef* expr) -> bool override {
//     ASSERT(expr);
//     NOT_IMPLEMENTED(ERROR);  // TODO: implement
//     return true;
//   }

//   auto VisitMacroDef(expr::MacroDef* expr) -> bool override {
//     ASSERT(expr);
//     NOT_IMPLEMENTED(ERROR);  // TODO: implement
//     return true;
//   }

//   auto VisitEvalExpr(expr::EvalExpr* expr) -> bool override {
//     ASSERT(expr);
//     NOT_IMPLEMENTED(ERROR);  // TODO: implement
//     return true;
//   }

//   auto VisitCallProcExpr(expr::CallProcExpr* expr) -> bool override {
//     ASSERT(expr);
//     NOT_IMPLEMENTED(ERROR);  // TODO: implement
//     return true;
//   }

//   auto VisitSetExpr(expr::SetExpr* expr) -> bool override {
//     ASSERT(expr);
//     NOT_IMPLEMENTED(ERROR);  // TODO: implement
//     return true;
//   }

//   auto VisitThrowExpr(expr::ThrowExpr* expr) -> bool override {
//     ASSERT(expr);
//     NOT_IMPLEMENTED(ERROR);  // TODO: implement
//     return true;
//   }
// };

// class MacroEvaluator : public expr::ExpressionVisitor {
//   DEFINE_NON_COPYABLE_TYPE(MacroEvaluator);

//  private:
//   MacroExpander* owner_;
//   LocalScope* scope_;
//   expr::Expression* result_ = nullptr;

//  public:
//   explicit MacroEvaluator(MacroExpander* owner, LocalScope* scope = LocalScope::New()) :
//     owner_(owner),
//     scope_(scope) {
//     ASSERT(owner_);
//     ASSERT(scope_);
//   }
//   ~MacroEvaluator() override = default;

//   auto GetOwner() const -> MacroExpander* {
//     return owner_;
//   }

//   auto GetScope() const -> LocalScope* {
//     return scope_;
//   }

//   auto GetResult() const -> expr::Expression* {
//     return result_;
//   }

//   inline auto HasResult() const -> bool {
//     return GetResult() != nullptr;
//   }

//   auto Eval(expr::Expression* expr) -> bool {
//     ASSERT(expr);
//     if (!expr->Accept(this)) {
//       LOG(FATAL) << "failed to eval: " << expr->ToString();
//       return false;
//     }
//     return true;
//   }

//   auto VisitBeginExpr(expr::BeginExpr* expr) -> bool override {
//     ASSERT(expr);
//     expr::ExpressionList exprs;
//     for (auto idx = 0; idx < expr->GetNumberOfChildren(); idx++) {
//       const auto child = expr->GetChildAt(idx);
//       ASSERT(child);

//       MacroEvaluator eval(GetOwner(), GetScope());
//       if (!child->Accept(&eval)) {
//         LOG(FATAL) << "failed to visit: " << child->ToString();
//         return false;
//       }

//       if (eval.HasResult()) {
//         const auto result = eval.GetResult();
//         ASSERT(result);
//         exprs.push_back(result);
//         DLOG(INFO) << "replaced " << child->ToString() << " w/ " << result->ToString();
//       } else if (!child->IsMacroDef()) {
//         exprs.push_back(child);
//       }
//     }
//     result_ = expr::BeginExpr::New(exprs);
//     return true;
//   }

//   auto VisitCaseExpr(expr::CaseExpr* expr) -> bool override {
//     ASSERT(expr);
//     NOT_IMPLEMENTED(ERROR);  // TODO: implement
//     return true;
//   }

//   auto VisitClauseExpr(expr::ClauseExpr* expr) -> bool override {
//     ASSERT(expr);
//     NOT_IMPLEMENTED(ERROR);  // TODO: implement
//     return true;
//   }

//   auto VisitConsExpr(expr::ConsExpr* expr) -> bool override {
//     ASSERT(expr);
//     {
//       // car
//       const auto value = expr->GetCar();
//       ASSERT(value);
//       MacroEvaluator eval(GetOwner());
//       if (!value->Accept(&eval)) {
//         LOG(FATAL) << "failed to visit: " << value->ToString();
//         return false;
//       }
//       if (eval.HasResult())
//         expr->SetCar(eval.GetResult());
//     }
//     {
//       // cdr
//       const auto value = expr->GetCdr();
//       ASSERT(value);
//       MacroEvaluator eval(GetOwner());
//       if (!value->Accept(&eval)) {
//         LOG(FATAL) << "failed to visit: " << value->ToString();
//         return false;
//       }
//       if (eval.HasResult())
//         expr->SetCdr(eval.GetResult());
//     }
//     return true;
//   }

//   auto VisitWhenExpr(expr::WhenExpr* expr) -> bool override {
//     ASSERT(expr);
//     NOT_IMPLEMENTED(ERROR);  // TODO: implement
//     return true;
//   }

//   auto VisitLambdaExpr(expr::LambdaExpr* expr) -> bool override {
//     ASSERT(expr);
//     NOT_IMPLEMENTED(ERROR);  // TODO: implement
//     return true;
//   }

//   auto VisitLiteralExpr(expr::LiteralExpr* expr) -> bool override {
//     ASSERT(expr);
//     NOT_IMPLEMENTED(ERROR);  // TODO: implement
//     return true;
//   }

//   auto VisitUnaryExpr(expr::UnaryExpr* expr) -> bool override {
//     ASSERT(expr);
//     const auto value = expr->GetValue();
//     ASSERT(value);
//     MacroEvaluator eval(GetOwner());
//     if (!value->Accept(&eval)) {
//       LOG(FATAL) << "failed to visit: " << value->ToString();
//       return false;
//     }
//     if (eval.HasResult())
//       expr->SetValue(eval.GetResult());
//     return true;
//   }

//   auto VisitWhileExpr(expr::WhileExpr* expr) -> bool override {
//     ASSERT(expr);
//     NOT_IMPLEMENTED(ERROR);  // TODO: implement
//     return true;
//   }

//   auto VisitBinaryOpExpr(expr::BinaryOpExpr* expr) -> bool override {
//     ASSERT(expr);
//     {
//       // left
//       const auto value = expr->GetLeft();
//       ASSERT(value);
//       MacroEvaluator eval(GetOwner());
//       if (!value->Accept(&eval)) {
//         LOG(FATAL) << "failed to visit: " << value->ToString();
//         return false;
//       }
//       if (eval.HasResult())
//         expr->SetLeft(eval.GetResult());
//     }
//     {
//       // right
//       const auto value = expr->GetRight();
//       ASSERT(value);
//       MacroEvaluator eval(GetOwner());
//       if (!value->Accept(&eval)) {
//         LOG(FATAL) << "failed to visit: " << value->ToString();
//         return false;
//       }
//       if (eval.HasResult())
//         expr->SetRight(eval.GetResult());
//     }
//     return true;
//   }

//   auto VisitCondExpr(expr::CondExpr* expr) -> bool override {
//     ASSERT(expr);
//     NOT_IMPLEMENTED(ERROR);  // TODO: implement
//     return true;
//   }

//   auto VisitQuotedExpr(expr::QuotedExpr* expr) -> bool override {
//     ASSERT(expr);
//     NOT_IMPLEMENTED(ERROR);  // TODO: implement
//     return true;
//   }

//   auto VisitLocalDef(expr::LocalDef* expr) -> bool override {
//     ASSERT(expr);
//     NOT_IMPLEMENTED(ERROR);  // TODO: implement
//     return true;
//   }

//   auto VisitModuleDef(expr::ModuleDef* expr) -> bool override {
//     ASSERT(expr);
//     NOT_IMPLEMENTED(ERROR);  // TODO: implement
//     return true;
//   }

//   auto VisitImportDef(expr::ImportDef* expr) -> bool override {
//     ASSERT(expr);
//     NOT_IMPLEMENTED(ERROR);  // TODO: implement
//     return true;
//   }

//   auto VisitMacroDef(expr::MacroDef* expr) -> bool override {
//     ASSERT(expr);

//     const auto symbol = expr->GetSymbol();
//     ASSERT(symbol);

//     LocalVariable* local = nullptr;
//     if (GetScope()->Lookup(symbol, &local, false)) {
//       LOG(FATAL) << "cannot redefine macro: " << symbol;
//       return false;
//     }

//     const auto macro = Macro::New(expr->GetSymbol(), expr->GetArgs(), expr->GetBody());
//     ASSERT(macro);
//     local = LocalVariable::New(GetScope(), symbol, macro);
//     ASSERT(local);
//     LOG_IF(FATAL, !GetScope()->Add(local)) << "failed to add local: " << (*local);
//     return true;
//   }

//   auto VisitEvalExpr(expr::EvalExpr* expr) -> bool override {
//     ASSERT(expr);
//     NOT_IMPLEMENTED(ERROR);  // TODO: implement
//     return true;
//   }

//   static inline auto LoadMacro(LocalScope* scope, Symbol* symbol, Macro** result) -> bool {
//     ASSERT(scope);
//     ASSERT(symbol);
//     LocalVariable* local = nullptr;
//     if (!scope->Lookup(symbol, &local)) {
//       (*result) = nullptr;
//       return false;
//     }
//     ASSERT(local);
//     if (!local->IsMacro()) {
//       DLOG(ERROR) << local->GetValue() << " is not a Macro.";
//       (*result) = nullptr;
//       return false;
//     }
//     const auto macro = local->GetValue()->AsMacro();
//     (*result) = macro;
//     return true;
//   }

//   static inline auto GetTargetSymbol(expr::CallProcExpr* expr, Symbol** result) -> bool {
//     ASSERT(expr);
//     const auto target = expr->GetTarget();
//     ASSERT(target);
//     if (!target->IsConstantExpr()) {
//       (*result) = nullptr;
//       return false;
//     }
//     const auto value = target->EvalToConstant();
//     ASSERT(value && value->IsSymbol());
//     (*result) = value->AsSymbol();
//     return true;
//   }

//   auto VisitCallProcExpr(expr::CallProcExpr* expr) -> bool override {
//     ASSERT(expr);
//     Symbol* target = nullptr;
//     if (!GetTargetSymbol(expr, &target)) {
//       LOG(ERROR) << "cannot get target Symbol for: " << expr->ToString();
//       return true;
//     }

//     Macro* macro = nullptr;
//     if (!LoadMacro(GetScope(), target, &macro)) {
//       return true;
//     }

//     ASSERT(macro);
//     if (!macro->HasBody())
//       return true;

//     const auto& args = macro->GetArgs();
//     if (expr->GetNumberOfArgs() != args.size()) {
//       LOG(FATAL) << "not the correct number of args.";
//       return false;
//     }

//     const auto scope = LocalScope::New(GetScope());
//     ASSERT(scope);

//     DLOG(INFO) << "args:";
//     auto idx = 0;
//     for (const auto& arg : macro->GetArgs()) {
//       const auto symbol = Symbol::New(arg.GetName());
//       const auto value = expr->GetArgAt(idx);
//       DLOG(INFO) << "- #" << (idx) << ": " << symbol << " - " << value->ToString();
//       scope->Add(LocalVariable::New(scope, symbol, value));
//     }

//     DLOG(INFO) << "macro scope: ";
//     LocalScopePrinter::Print(scope, __FILE__, __LINE__);

//     const auto body = macro->GetBody();
//     ASSERT(body);
//     MacroReplacer replacer(GetOwner(), scope);
//     if (!body->Accept(&replacer)) {
//       return false;
//     }

//     if (replacer.HasResult())
//       result_ = replacer.GetResult();
//     DLOG_IF(INFO, !replacer.HasResult()) << "no replacement result for " << expr->ToString();
//     return true;
//   }

//   auto VisitSetExpr(expr::SetExpr* expr) -> bool override {
//     ASSERT(expr);
//     NOT_IMPLEMENTED(ERROR);  // TODO: implement
//     return true;
//   }

//   auto VisitThrowExpr(expr::ThrowExpr* expr) -> bool override {
//     ASSERT(expr);
//     NOT_IMPLEMENTED(ERROR);  // TODO: implement
//     return true;
//   }
// };

// auto MacroExpander::Expand(expr::Expression** expr) -> bool {
//   ASSERT((*expr));
//   MacroEvaluator eval(this);
//   if (!(*expr)->Accept(&eval)) {
//     LOG(ERROR) << "failed to expand: " << (*expr)->ToString();
//     return false;
//   }

//   if (!eval.HasResult())
//     return true;
//   (*expr) = eval.GetResult();
//   return true;
// }
}  // namespace scm