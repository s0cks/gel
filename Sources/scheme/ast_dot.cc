#include "scheme/ast_dot.h"

#include <glog/logging.h>

#include "scheme/common.h"

namespace scm::ast {
auto NodeToDot::VisitProgram(Program* p) -> bool {
  const auto node = CreateNewNode();
  ASSERT(node);
  SetNodeLabel(node, "Program()");
  SetParent(node);
  return p->VisitAllForms(this);
}

auto NodeToDot::VisitBeginDef(BeginDef* n) -> bool {
  const auto old_parent = GetParent();

  const auto node = CreateNewNode();
  ASSERT(node);
  SetNodeLabel(node, "Begin()");
  SetParent(node);
  if (!n->VisitChildren(this)) {
    // TODO: delete node
    SetParent(old_parent);
    return false;
  }
  SetParent(old_parent);

  if (HasParent()) {
    const auto edge = NewEdge(GetParent(), node);
    ASSERT(edge);
  }
  return true;
}

auto NodeToDot::VisitBinaryOpExpr(BinaryOpExpr* p) -> bool {
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return true;
}

auto NodeToDot::VisitBody(Body* p) -> bool {
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return true;
}

auto NodeToDot::VisitVariableDef(VariableDef* n) -> bool {
  ASSERT(n);
  const auto old_parent = GetParent();

  const auto var = n->GetVar();
  const auto node = CreateNewNode();
  ASSERT(node);
  SetNodeLabel(node, fmt::format("VariableDef(var={0:s})", var->GetName()));
  SetParent(node);
  if (!n->GetVal()->Accept(this)) {
    // TODO: delete node
    SetParent(old_parent);
    return false;
  }
  SetParent(old_parent);

  if (HasParent()) {
    const auto edge = NewEdge(GetParent(), node);
    ASSERT(edge);
  }
  return true;
}

auto NodeToDot::VisitSyntaxDef(SyntaxDef* p) -> bool {
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return true;
}

auto NodeToDot::VisitExpressionList(ExpressionList* p) -> bool {
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return true;
}

auto NodeToDot::VisitQuoteExpr(QuoteExpr* p) -> bool {
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return true;
}

auto NodeToDot::VisitCallProcExpr(CallProcExpr* p) -> bool {
  const auto node = CreateNewNode();
  ASSERT(node);
  SetNodeLabel(node, fmt::format("CallProcExpr(symbol={0:s})", p->GetSymbol()->Get()));
  if (HasParent()) {
    const auto edge = NewEdge(GetParent(), node);
    ASSERT(edge);
  }
  return true;
}

auto NodeToDot::VisitConstantExpr(ConstantExpr* expr) -> bool {
  ASSERT(expr);
  const auto value = expr->GetValue();
  ASSERT(value);
  const auto node = CreateNewNode();
  ASSERT(node);
  SetNodeLabel(node, value->ToString());
  if (HasParent()) {
    const auto edge = NewEdge(GetParent(), node);
    ASSERT(edge);
  }
  return true;
}

auto NodeToDot::VisitLoadVariableExpr(LoadVariableExpr* p) -> bool {
  return true;
}

auto NodeToDot::BuildDotGraph() -> DotGraph* {
  ASSERT(HasProgram());
  SetNodeAttr("label", "");
  SetNodeAttr("xlabel", "");
  if (!GetProgram()->Accept(this))
    return nullptr;
  return DotGraph::New(GetGraph());
}
}  // namespace scm::ast