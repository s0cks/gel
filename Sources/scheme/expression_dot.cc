#include "scheme/expression_dot.h"

#include <glog/logging.h>

#include <algorithm>

#include "scheme/common.h"
#include "scheme/expression.h"

namespace scm {
ExpressionToDot::ExpressionToDot(const char* graph_name) :
  dot::GraphBuilder(graph_name, Agdirected) {
  SetNodeAttr("label", "");
  SetNodeAttr("xlabel", "");
}

static inline auto ToString(Datum* datum) -> std::string {
  ASSERT(datum);
  std::stringstream ss;
  if (datum->IsNumber()) {
    ss << datum->AsNumber()->GetValue();
  } else if (datum->IsSymbol()) {
    ss << datum->AsSymbol()->Get();
  } else {
    ss << datum->ToString();
  }
  return ss.str();
}

auto ExpressionToDot::VisitDefine(DefineExpr* expr) -> bool {
  ASSERT(expr);
  // create new node
  const auto node = NewNode();
  ASSERT(node);
  {
    // set label
    const auto symbol = expr->GetSymbol();
    ASSERT(symbol);
    std::stringstream label;
    label << expr->GetName() << "Expr" << std::endl;
    label << "Symbol := " << ToString(symbol);
    SetNodeLabel(node, label);
  }
  {
    // process children
    NodeScope scope(this, node);
    const auto value = expr->GetValue();
    ASSERT(value);
    if (!value->Accept(this)) {
      LOG(ERROR) << "failed to visit: " << value->ToString();
      return false;
    }
  }
  CreateEdgeFromParent(node);
  return true;
}

auto ExpressionToDot::VisitBegin(BeginExpr* expr) -> bool {
  ASSERT(expr);
  // create new node
  const auto node = NewNode();
  ASSERT(node);
  {
    // create node labels
    // label
    std::stringstream label;
    label << expr->GetName() << "Expr" << std::endl;
    SetNodeLabel(node, label);
    // xlabel
    std::stringstream xlabel;
    xlabel << expr->GetNumberOfChildren() << " expressions";
    SetNodeXLabel(node, xlabel);
  }
  {
    // process children
    NodeScope scope(this, node);
    if (!expr->VisitChildren(this)) {
      LOG(ERROR) << "failed to visit children of: " << expr->ToString();
      return false;
    }
  }
  CreateEdgeFromParent(node);
  return true;
}

auto ExpressionToDot::VisitBinaryOp(BinaryOpExpr* expr) -> bool {
  ASSERT(expr);
  const auto node = NewNode();
  ASSERT(node);
  {
    // create node labels
    std::stringstream label;
    label << expr->GetName() << "Expr" << std::endl;
    label << "Op: " << expr->GetOp();
    SetNodeLabel(node, label);
  }
  {
    // process children
    NodeScope scope(this, node);
    if (!expr->VisitChildren(this)) {
      LOG(ERROR) << "failed to visit children of: " << expr->ToString();
      return false;
    }
  }
  CreateEdgeFromParent(node);
  return true;
}

auto ExpressionToDot::VisitEval(EvalExpr* expr) -> bool {
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return true;
}

auto ExpressionToDot::VisitCallProc(CallProcExpr* expr) -> bool {
  ASSERT(expr);
  // create new node
  const auto node = NewNode();
  ASSERT(node);
  {
    // create node labels
    // label
    const auto symbol = expr->GetSymbol();
    ASSERT(symbol);
    std::stringstream label;
    label << expr->GetName() << "Expr" << std::endl;
    label << "Symbol := " << symbol->Get();
    SetNodeLabel(node, label);
  }
  {
    // process children
    NodeScope scope(this, node);
    if (!expr->VisitChildren(this)) {
      LOG(ERROR) << "failed to visit children of: " << expr->ToString();
      return false;
    }
  }
  CreateEdgeFromParent(node);
  return true;
}

auto ExpressionToDot::VisitSymbol(SymbolExpr* expr) -> bool {
  ASSERT(expr);
  const auto node = NewNode();
  ASSERT(node);
  {
    // create node labels
    // label
    std::stringstream label;
    label << expr->GetName() << "Expr" << std::endl;
    label << "Symbol := " << expr->GetSymbol()->Get();
    SetNodeLabel(node, label);
  }
  CreateEdgeFromParent(node);
  return true;
}

auto ExpressionToDot::VisitLiteral(LiteralExpr* expr) -> bool {
  ASSERT(expr);
  const auto node = NewNode();
  ASSERT(node);
  {
    // create node labels
    // label
    std::stringstream label;
    label << expr->GetName() << "Expr" << std::endl;
    const auto value = expr->GetValue();
    ASSERT(value);
    label << "Value := " << ToString(value);
    SetNodeLabel(node, label);
  }
  CreateEdgeFromParent(node);
  return true;
}

auto ExpressionToDot::Build() -> dot::Graph* {
  return GetGraph();
}
}  // namespace scm