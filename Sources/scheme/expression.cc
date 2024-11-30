#include "scheme/expression.h"

#include <glog/logging.h>

#include <algorithm>
#include <sstream>
#include <string>

#include "scheme/common.h"
#include "scheme/heap.h"
#include "scheme/natives.h"
#include "scheme/object.h"
#include "scheme/to_string_helper.h"

namespace scm::expr {
Class* Expression::kClass = nullptr;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
void Expression::Init() {
  ASSERT(kClass == nullptr);
  kClass = Class::New(Object::GetClass(), kClassName);
  ASSERT(kClass);
}

#ifdef SCM_DISABLE_HEAP

#define DEFINE_NEW_OPERATOR(Name)                     \
  auto Name::operator new(const size_t sz) -> void* { \
    return malloc(sz);                                \
  }

#else

#define DEFINE_NEW_OPERATOR(Name)                     \
  auto Name::operator new(const size_t sz) -> void* { \
    const auto heap = Heap::GetHeap();                \
    ASSERT(heap);                                     \
    const auto address = heap->TryAllocate(sz);       \
    ASSERT(address != UNALLOCATED);                   \
    return reinterpret_cast<void*>(address);          \
  }

#endif  // SCM_DISABLE_HEAP

FOR_EACH_EXPRESSION_NODE(DEFINE_NEW_OPERATOR)  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
#undef DEFINE_NEW_OPERATOR

#define DEFINE_ACCEPT(Name)                           \
  auto Name::Accept(ExpressionVisitor* vis) -> bool { \
    ASSERT(vis);                                      \
    return vis->Visit##Name(this);                    \
  }
FOR_EACH_EXPRESSION_NODE(DEFINE_ACCEPT)
#undef DEFINE_ACCEPT

auto SequenceExpr::IsConstantExpr() const -> bool {
  const auto found = std::ranges::find_if(std::begin(children_), std::end(children_), [](Expression* expr) {
    return !expr->IsConstantExpr();
  });
  return found != std::end(children_);
}

auto SequenceExpr::VisitChildren(ExpressionVisitor* vis) -> bool {
  ASSERT(vis);
  for (const auto& expr : children_) {
    if (!expr->Accept(vis))
      return false;
  }
  return true;
}

auto SequenceExpr::VisitAllDefinitions(ExpressionVisitor* vis) -> bool {
  ASSERT(vis);
  for (const auto& expr : children_) {
    if (expr->IsDefinition()) {
      if (!expr->Accept(vis))
        return false;
    }
  }
  return true;
}

auto LiteralExpr::ToString() const -> std::string {
  ToStringHelper<LiteralExpr> helper;
  helper.AddField("value", GetValue());
  return helper;
}

auto BinaryOpExpr::IsConstantExpr() const -> bool {
  return GetLeft()->IsConstantExpr() && GetRight()->IsConstantExpr();
}

auto BinaryOpExpr::EvalToConstant() const -> Object* {
  ASSERT(IsConstantExpr());
  const auto left = GetLeft()->EvalToConstant();
  ASSERT(left && left->IsAtom());
  const auto right = GetRight()->EvalToConstant();
  ASSERT(right && right->IsAtom());
  switch (GetOp()) {
    case BinaryOp::kAdd:
      return dynamic_cast<Datum*>(left)->Add(dynamic_cast<Datum*>(right));
    case BinaryOp::kSubtract:
      return dynamic_cast<Datum*>(left)->Sub(dynamic_cast<Datum*>(right));
    case BinaryOp::kMultiply:
      return dynamic_cast<Datum*>(left)->Mul(dynamic_cast<Datum*>(right));
    case BinaryOp::kDivide:
      return dynamic_cast<Datum*>(left)->Div(dynamic_cast<Datum*>(right));
    case BinaryOp::kModulus:
      return dynamic_cast<Datum*>(left)->Mod(dynamic_cast<Datum*>(right));
    default:
      LOG(FATAL) << "invalid binary op: " << GetOp();
      return nullptr;
  }
}

auto BinaryOpExpr::VisitChildren(ExpressionVisitor* vis) -> bool {
  ASSERT(vis);
  if (!GetLeft()->Accept(vis))
    return false;
  if (!GetRight()->Accept(vis))
    return false;
  return true;
}

auto BinaryOpExpr::ToString() const -> std::string {
  ToStringHelper<BinaryOpExpr> helper;
  helper.AddField("op", GetOp());
  helper.AddField("left", GetLeft());
  helper.AddField("right", GetRight());
  return helper;
}

auto EvalExpr::ToString() const -> std::string {
  ToStringHelper<EvalExpr> helper;
  if (HasExpression())
    helper.AddField("expression", GetExpression());
  return helper;
}

auto BeginExpr::ToString() const -> std::string {
  ToStringHelper<EvalExpr> helper;
  if (!IsEmpty())
    helper.AddField("num_expressions", GetNumberOfChildren());
  return helper;
}

auto CallProcExpr::ToString() const -> std::string {
  ToStringHelper<CallProcExpr> helper;
  helper.AddField("target", GetTarget());
  helper.AddField("num_args", GetNumberOfArgs());
  return helper;
}

auto SetExpr::ToString() const -> std::string {
  ToStringHelper<SetExpr> helper;
  helper.AddField("symbol", GetSymbol());
  helper.AddField("value", GetValue());
  return helper;
}

auto CondExpr::VisitAllClauses(ExpressionVisitor* vis) -> bool {
  ASSERT(vis);
  for (const auto& clause : clauses_) {
    ASSERT(clause);
    if (!clause->Accept(vis))
      return false;
  }
  return true;
}

auto CondExpr::VisitChildren(ExpressionVisitor* vis) -> bool {
  ASSERT(vis);
  if (!VisitAllClauses(vis))
    return false;
  if (HasAlternate()) {
    if (!GetAlternate()->Accept(vis))
      return false;
  }
  return true;
}

auto CondExpr::ToString() const -> std::string {
  ToStringHelper<CondExpr> helper;
  helper.AddField("clauses", GetClauses());
  helper.AddField("alternate", GetAlternate());
  return helper;
}

auto LambdaExpr::VisitChildren(ExpressionVisitor* vis) -> bool {
  ASSERT(vis);
  for (const auto& expr : body_) {
    if (!expr->Accept(vis))
      return false;
  }
  return true;
}

auto LambdaExpr::ToString() const -> std::string {
  ToStringHelper<LambdaExpr> helper;
  helper.AddField("args", GetArgs());
  helper.AddField("body", GetBody());
  return helper;
}

auto ThrowExpr::ToString() const -> std::string {
  ToStringHelper<ThrowExpr> helper;
  helper.AddField("value", GetValue());
  return helper;
}

// Definitions

auto LocalDef::ToString() const -> std::string {
  ToStringHelper<LocalDef> helper;
  helper.AddField("symbol", GetSymbol());
  helper.AddField("value", GetValue());
  return helper;
}

auto ImportDef::ToString() const -> std::string {
  ToStringHelper<ImportDef> helper;
  helper.AddField("symbol", GetSymbol());
  return helper;
}

auto MacroDef::ToString() const -> std::string {
  ToStringHelper<MacroDef> helper;
  helper.AddField("symbol", GetSymbol());
  helper.AddField("body", GetBody());
  return helper;
}

auto UnaryExpr::ToString() const -> std::string {
  ToStringHelper<UnaryExpr> helper;
  helper.AddField("op", GetOp());
  helper.AddField("value", GetValue());
  return helper;
}

auto QuotedExpr::ToString() const -> std::string {
  ToStringHelper<QuotedExpr> helper;
  helper.AddField("value", Get());
  return helper;
}

auto ClauseExpr::ToString() const -> std::string {
  ToStringHelper<ClauseExpr> helper;
  helper.AddField("key", GetKey());
  helper.AddField("actions", GetActions());
  return helper;
}

auto ClauseExpr::VisitAllActions(ExpressionVisitor* vis) -> bool {
  ASSERT(vis);
  for (const auto& action : actions_) {
    ASSERT(action);
    if (!action->Accept(vis))
      return false;
  }
  return true;
}

auto ClauseExpr::VisitChildren(ExpressionVisitor* vis) -> bool {
  ASSERT(vis);
  if (!GetKey()->Accept(vis))
    return false;
  return VisitAllActions(vis);
}

auto WhenExpr::VisitChildren(ExpressionVisitor* vis) -> bool {
  ASSERT(vis);
  if (!GetTest()->Accept(vis))
    return false;
  for (const auto& action : GetActions()) {
    if (!action->Accept(vis))
      return false;
  }
  return true;
}

auto CaseExpr::VisitAllClauses(ExpressionVisitor* vis) -> bool {
  ASSERT(vis);
  for (const auto& clause : clauses_) {
    ASSERT(clause);
    if (!clause->Accept(vis))
      return false;
  }
  return true;
}

auto CaseExpr::VisitChildren(ExpressionVisitor* vis) -> bool {
  ASSERT(vis);
  if (!GetKey()->Accept(vis))
    return false;
  return VisitAllClauses(vis);
}

auto CaseExpr::ToString() const -> std::string {
  ToStringHelper<CaseExpr> helper;
  helper.AddField("key", GetKey());
  helper.AddField("clauses", GetClauses());
  return helper;
}

auto WhenExpr::ToString() const -> std::string {
  ToStringHelper<WhenExpr> helper;
  helper.AddField("test", GetTest());
  helper.AddField("actions", GetActions());
  return helper;
}

auto WhileExpr::ToString() const -> std::string {
  ToStringHelper<WhileExpr> helper;
  helper.AddField("test", GetTest());
  helper.AddField("body", GetBody());
  return helper;
}

auto Binding::ToString() const -> std::string {
  ToStringHelper<Binding> helper;
  helper.AddField("symbol", GetSymbol());
  helper.AddField("value", GetValue());
  return helper;
}

auto RxOpExpr::ToString() const -> std::string {
  ToStringHelper<RxOpExpr> helper;
  helper.AddField("symbol", GetSymbol());
  helper.AddField("args", GetBody());
  return helper;
}

auto RxOpExpr::IsSubscribe() const -> bool {
  return IsCallToNative<proc::rx_subscribe>(GetSymbol());
}

auto LetRxExpr::ToString() const -> std::string {
  ToStringHelper<LetRxExpr> helper;
  helper.AddField("scope", GetScope());
  helper.AddField("body", GetBody());
  return helper;
}

auto LetRxExpr::HasSubscribe() const -> bool {
  const auto last = GetLastOp();
  if (!last)
    return last;
  return last->IsSubscribe();
}

auto LetExpr::ToString() const -> std::string {
  ToStringHelper<LetExpr> helper;
  helper.AddField("scope", GetScope());
  helper.AddField("binings", GetBindings());
  helper.AddField("body", GetBody());
  return helper;
}

auto InstanceOfExpr::ToString() const -> std::string {
  ToStringHelper<InstanceOfExpr> helper;
  helper.AddField("expected", GetExpected());
  helper.AddField("actual", GetActual());
  return helper;
}

auto InstanceOfExpr::EvalToConstant() const -> Object* {
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return nullptr;
}

auto InstanceOfExpr::IsConstantExpr() const -> bool {
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return false;
}

auto LetExpr::VisitAllBindings(ExpressionVisitor* vis) -> bool {
  ASSERT(vis);
  for (const auto& binding : bindings_) {
    ASSERT(binding);
    if (!binding->Accept(vis))
      return false;
  }
  return true;
}

auto LetExpr::VisitChildren(ExpressionVisitor* vis) -> bool {
  ASSERT(vis);
  if (!VisitAllBindings(vis))
    return false;
  return SequenceExpr::VisitChildren(vis);
}

auto ListExpr::ToString() const -> std::string {
  ToStringHelper<ListExpr> helper;
  helper.AddField("values", GetBody());
  return helper;
}

auto ListExpr::IsConstantExpr() const -> bool {
  if (IsEmpty())
    return true;
  for (auto idx = 0; idx < GetNumberOfChildren(); idx++) {
    const auto child = GetChildAt(idx);
    ASSERT(child);
    if (!child->IsConstantExpr())
      return false;
  }
  return true;
}

auto ListExpr::EvalToConstant() const -> Object* {
  ASSERT(IsConstantExpr());
  if (IsEmpty())
    return Pair::Empty();
  Object* value = Pair::Empty();
  for (auto idx = GetNumberOfChildren(); idx > 0; idx--) {
    const auto child = GetChildAt(idx - 1);
    ASSERT(child && child->IsConstantExpr());
    value = scm::Cons(child->EvalToConstant(), value);
  }
  return value;
}
}  // namespace scm::expr