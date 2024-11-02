#include "scheme/expression.h"

#include <glog/logging.h>

#include <algorithm>
#include <sstream>
#include <string>

#include "scheme/common.h"

namespace scm::expr {
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
  std::stringstream ss;
  ss << GetName() << "(";
  ss << "value=" << GetValue()->ToString();
  ss << ")";
  return ss.str();
}

auto BinaryOpExpr::IsConstantExpr() const -> bool {
  return GetLeft()->IsConstantExpr() && GetRight()->IsConstantExpr();
}

auto BinaryOpExpr::EvalToConstant() const -> Type* {
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

auto ConsExpr::IsConstantExpr() const -> bool {
  return GetCar()->IsConstantExpr() && GetCdr()->IsConstantExpr();
}

auto ConsExpr::EvalToConstant() const -> Type* {
  ASSERT(IsConstantExpr());
  const auto car = GetCar()->EvalToConstant();
  ASSERT(car);
  const auto cdr = GetCdr()->EvalToConstant();
  ASSERT(cdr);
  return Pair::New(car, cdr);
}

auto ConsExpr::ToString() const -> std::string {
  std::stringstream ss;
  ss << GetName() << "(";
  ss << "car=" << GetCar()->ToString() << ", ";
  ss << "cdr=" << GetCdr()->ToString();
  ss << ")";
  return ss.str();
}

auto BinaryOpExpr::ToString() const -> std::string {
  std::stringstream ss;
  ss << GetName() << "(";
  ss << "op=" << GetOp() << ", ";
  ss << "left=" << GetLeft()->ToString() << ", ";
  ss << "right=" << GetRight()->ToString();
  ss << ")";
  return ss.str();
}

auto EvalExpr::ToString() const -> std::string {
  std::stringstream ss;
  ss << GetName() << "(";
  if (HasExpression())
    ss << "expr=" << GetExpression()->ToString();
  ss << ")";
  return ss.str();
}

auto BeginExpr::ToString() const -> std::string {
  std::stringstream ss;
  ss << GetName() << "(";
  ss << "num_expressions=" << GetNumberOfChildren();
  ss << ")";
  return ss.str();
}

auto CallProcExpr::ToString() const -> std::string {
  std::stringstream ss;
  ss << GetName() << "(";
  ss << "target=" << GetTarget()->ToString() << ", ";
  ss << "num_args=" << GetNumberOfArgs();
  ss << ")";
  return ss.str();
}

auto SetExpr::ToString() const -> std::string {
  std::stringstream ss;
  ss << GetName() << "(";
  ss << "symbol=" << GetSymbol() << ", ";
  ss << "value=" << GetValue();
  ss << ")";
  return ss.str();
}

auto CondExpr::VisitChildren(ExpressionVisitor* vis) -> bool {
  ASSERT(vis);
  if (!GetTest()->Accept(vis))
    return false;
  if (!GetConseq()->Accept(vis))
    return false;
  if (HasAlternate()) {
    if (!GetAlternate()->Accept(vis))
      return false;
  }
  return true;
}

auto CondExpr::ToString() const -> std::string {
  std::stringstream ss;
  ss << GetName() << "(";
  ss << "test=" << GetTest() << ", ";
  ss << "consequent=" << GetConseq() << ", ";
  ss << "alternate=" << GetAlternate();
  ss << ")";
  return ss.str();
}

auto LambdaExpr::VisitChildren(ExpressionVisitor* vis) -> bool {
  ASSERT(vis);
  if (!HasBody())
    return true;
  if (!GetBody()->Accept(vis))
    return false;
  return true;
}

auto LambdaExpr::ToString() const -> std::string {
  std::stringstream ss;
  ss << GetName() << "(";
  ss << "args=" << GetArgs();
  if (HasBody())
    ss << ", body=" << GetBody()->ToString();
  ss << ")";
  return ss.str();
}

auto ThrowExpr::ToString() const -> std::string {
  std::stringstream ss;
  ss << GetName() << "(";
  ss << "value=" << GetValue()->ToString();
  ss << ")";
  return ss.str();
}

// Definitions

auto LocalDef::ToString() const -> std::string {
  std::stringstream ss;
  ss << GetName() << "(";
  ss << "symbol=" << GetSymbol() << ", ";
  ss << "value=" << GetValue()->ToString();
  ss << ")";
  return ss.str();
}

auto ImportDef::ToString() const -> std::string {
  std::stringstream ss;
  ss << GetName() << "(";
  ss << "symbol=" << GetSymbol();
  ss << ")";
  return ss.str();
}

auto ModuleDef::VisitChildren(ExpressionVisitor* vis) -> bool {
  ASSERT(vis);
  for (const auto& defn : definitions_) {
    if (!defn->Accept(vis))
      return false;
  }
  return true;
}

auto ModuleDef::VisitAllImportDefs(ExpressionVisitor* vis) -> bool {
  ASSERT(vis);
  for (const auto& defn : definitions_) {
    if (defn->IsImportDef()) {
      if (!defn->Accept(vis))
        return false;
    }
  }
  return true;
}

auto ModuleDef::ToString() const -> std::string {
  std::stringstream ss;
  ss << GetName() << "(";
  ss << "symbol=" << GetSymbol() << ", ";
  ss << "definitions=" << definitions_;
  ss << ")";
  return ss.str();
}

auto MacroDef::ToString() const -> std::string {
  std::stringstream ss;
  ss << GetName() << "(";
  ss << "symbol=" << GetSymbol() << ", ";
  ss << "body=" << GetBody()->ToString();
  ss << ")";
  return ss.str();
}

auto UnaryExpr::ToString() const -> std::string {
  std::stringstream ss;
  ss << GetName() << "(";
  ss << "op=" << GetOp() << ", ";
  ss << "value=" << GetValue()->ToString();
  ss << ")";
  return ss.str();
}

auto QuotedExpr::ToString() const -> std::string {
  std::stringstream ss;
  ss << "QuotedExpr(";
  ss << "value=" << Get();
  ss << ")";
  return ss.str();
}

auto ClauseExpr::ToString() const -> std::string {
  std::stringstream ss;
  ss << "ClauseExpr(";
  ss << "key=" << GetKey()->ToString() << ", ";
  ss << "actions=" << GetActions();
  ss << ")";
  return ss.str();
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
  std::stringstream ss;
  ss << "WhenExpr(";
  ss << "key=" << GetKey()->ToString() << ", ";
  // TODO: ss << "clauses=" << GetClauses();
  ss << ")";
  return ss.str();
}

auto WhenExpr::ToString() const -> std::string {
  std::stringstream ss;
  ss << "WhenExpr(";
  ss << "value=" << GetTest()->ToString() << ", ";
  ss << "actions=" << GetActions();
  ss << ")";
  return ss.str();
}

auto WhileExpr::ToString() const -> std::string {
  std::stringstream ss;
  ss << "LoopExpr(";
  ss << "body=" << GetBody();
  ss << ")";
  return ss.str();
}
}  // namespace scm::expr