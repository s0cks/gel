#include "scheme/ast.h"

#include <sstream>

namespace scm::ast {
auto Value::ToString() const -> std::string {
  std::stringstream ss;
  ss << "Value(";
  ss << "value=" << GetValue()->ToString();
  ss << ")";
  return ss.str();
}

#define DEFINE_ACCEPT(Name)                     \
  auto Name::Accept(NodeVisitor* vis) -> bool { \
    ASSERT(vis);                                \
    return vis->Visit##Name(this);              \
  }
FOR_EACH_AST_NODE(DEFINE_ACCEPT)
#undef DEFINE_ACCEPT

auto Program::VisitAllForms(NodeVisitor* vis) -> bool {
  ASSERT(vis);
  for (const auto& form : forms_) {
    if (!form->Accept(vis))
      return false;
  }
  return true;
}

auto Program::VisitAllDefinitions(NodeVisitor* vis) -> bool {
  ASSERT(vis);
  for (const auto& form : GetForms()) {
    if (form->IsDefinition() && !form->Accept(vis))
      return false;
  }
  return true;
}

auto Program::VisitAllExpressions(NodeVisitor* vis) -> bool {
  ASSERT(vis);
  for (const auto& form : GetForms()) {
    if (form->IsDefinition() && !form->Accept(vis))
      return false;
  }
  return true;
}

auto Program::ToString() const -> std::string {
  std::stringstream ss;
  ss << "Program(";
  ss << "num_forms=" << GetForms().size();
  if (!IsEmpty())
    ss << ", forms=" << GetForms();
  ss << ")";
  return ss.str();
}

auto VariableDef::ToString() const -> std::string {
  std::stringstream ss;
  ss << "VariableDef(";
  ss << "var=" << GetVar() << ", ";
  ss << "val=" << GetVal();
  ss << ")";
  return ss.str();
}

auto Body::ToString() const -> std::string {
  std::stringstream ss;
  ss << "Body(";
  ss << ")";
  return ss.str();
}

auto ConstantExpr::ToString() const -> std::string {
  std::stringstream ss;
  ss << "ConstantExpr(";
  ss << "value=" << GetValue()->ToString();
  ss << ")";
  return ss.str();
}

auto BeginDef::ToString() const -> std::string {
  std::stringstream ss;
  ss << "BeginDef(";
  ss << "definitions=" << GetDefinitions();
  ss << ")";
  return ss.str();
}

auto SyntaxDef::ToString() const -> std::string {
  std::stringstream ss;
  ss << "SyntaxDef(";
  ss << ")";
  return ss.str();
}

auto QuoteExpr::ToString() const -> std::string {
  std::stringstream ss;
  ss << "QuoteExpr(";
  ss << ")";
  return ss.str();
}

auto LoadVariableExpr::ToString() const -> std::string {
  std::stringstream ss;
  ss << "LoadVariableExpr(";
  ss << "var=" << GetVariable()->ToString();
  ss << ")";
  return ss.str();
}

auto CallProcExpr::ToString() const -> std::string {
  std::stringstream ss;
  ss << "CallProcExpr(";
  ss << "symbol=" << GetSymbol() << ", ";
  ss << "value=" << GetArgs()->ToString();
  ss << ")";
  return ss.str();
}

auto ExpressionList::ToString() const -> std::string {
  std::stringstream ss;
  ss << "ExpressionList(";
  ss << "expressions=";
  ss << "[";
  auto remaining = expressions_.size();
  for (const auto& expr : expressions_) {
    ss << expr->ToString();
    if (--remaining >= 1)
      ss << ", ";
  }
  ss << "]", ss << ")";
  return ss.str();
}

auto BinaryOpExpr::VisitChildren(NodeVisitor* vis) -> bool {
  ASSERT(vis);
  if (!GetLeft()->Accept(vis))
    return false;
  if (!GetRight()->Accept(vis))
    return false;
  return true;
}

auto BinaryOpExpr::ToString() const -> std::string {
  std::stringstream ss;
  ss << "BinaryOpExpr(";
  ss << "op=" << GetOp() << ", ";
  ss << "left=" << GetLeft()->ToString() << ", ";
  ss << "right=" << GetRight()->ToString();
  ss << ")";
  return ss.str();
}
}  // namespace scm::ast