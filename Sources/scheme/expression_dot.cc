#include "scheme/expression_dot.h"

#include <glog/logging.h>

#include <algorithm>

#include "scheme/common.h"
#include "scheme/expression.h"

namespace scm {
ExpressionToDot::ExpressionToDot(const char* graph_name) :
  dot::GraphBuilder(graph_name) {
  SetNodeAttr("label", "");
  SetNodeAttr("xlabel", "");
}

static inline auto ToString(Datum* datum) -> std::string {
  ASSERT(datum);
  std::stringstream ss;
  if (datum->IsLong()) {
    ss << datum->AsLong()->Get();
  } else if (datum->IsDouble()) {
    ss << datum->AsDouble()->Get();
  } else if (datum->IsSymbol()) {
    ss << datum->AsSymbol()->Get();
  } else {
    ss << datum->ToString();
  }
  return ss.str();
}

auto ExpressionToDot::VisitLocalDef(LocalDef* expr) -> bool {
  ASSERT(expr);
  // create new node
  const auto node = NewNode();
  ASSERT(node);
  {
    // set label
    const auto symbol = expr->GetSymbol();
    ASSERT(symbol);
    std::stringstream label;
    label << expr->GetName() << std::endl;
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

auto ExpressionToDot::VisitBeginExpr(BeginExpr* expr) -> bool {
  ASSERT(expr);
  // create new node
  const auto node = NewNode();
  ASSERT(node);
  {
    // create node labels
    // label
    std::stringstream label;
    label << expr->GetName() << std::endl;
    SetNodeLabel(node, label);
    // xlabel
    std::stringstream xlabel;
    xlabel << expr->GetNumberOfChildren() << " expressions";
    SetNodeXLabel(node, xlabel);
  }
  CreateEdgeFromParent(node);
  {
    // process children
    NodeScope scope(this, node);
    if (!expr->VisitChildren(this)) {
      LOG(ERROR) << "failed to visit children of: " << expr->ToString();
      return false;
    }
  }
  return true;
}

auto ExpressionToDot::VisitBinaryOpExpr(BinaryOpExpr* expr) -> bool {
  ASSERT(expr);
  const auto node = NewNode();
  ASSERT(node);
  {
    // create node labels
    std::stringstream label;
    label << expr->GetName() << std::endl;
    label << "Op: " << expr->GetOp();
    SetNodeLabel(node, label);
  }
  CreateEdgeFromParent(node);
  {
    // process children
    NodeScope scope(this, node);
    if (!expr->VisitChildren(this)) {
      LOG(ERROR) << "failed to visit children of: " << expr->ToString();
      return false;
    }
  }
  return true;
}

auto ExpressionToDot::VisitEvalExpr(EvalExpr* expr) -> bool {
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return true;
}

auto ExpressionToDot::VisitConsExpr(ConsExpr* expr) -> bool {
  ASSERT(expr);
  const auto node = NewNode();
  ASSERT(node);
  {
    // create node labels
    std::stringstream label;
    label << expr->GetName() << std::endl;
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

auto ExpressionToDot::VisitCallProcExpr(CallProcExpr* expr) -> bool {
  ASSERT(expr);
  // create new node
  const auto node = NewNode();
  ASSERT(node);
  {
    // create node labels
    // label
    std::stringstream label;
    label << expr->GetName() << std::endl;
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

auto ExpressionToDot::VisitLiteralExpr(LiteralExpr* expr) -> bool {
  ASSERT(expr);
  const auto node = NewNode();
  ASSERT(node);
  {
    // create node labels
    // label
    std::stringstream label;
    label << expr->GetName() << std::endl;
    const auto value = expr->GetValue();
    ASSERT(value);
    label << "Value := " << ToString(value);
    SetNodeLabel(node, label);
  }
  CreateEdgeFromParent(node);
  return true;
}

auto ExpressionToDot::VisitUnaryExpr(UnaryExpr* expr) -> bool {
  ASSERT(expr);
  const auto node = NewNode();
  ASSERT(node);
  {
    // create node labels
    // label
    std::stringstream label;
    label << expr->GetName() << std::endl;
    label << "Op := " << expr->GetOp();
    SetNodeLabel(node, label);
  }
  CreateEdgeFromParent(node);
  {
    // process children
    NodeScope scope(this, node);
    if (!expr->VisitChildren(this)) {
      LOG(ERROR) << "failed to visit children of: " << expr->ToString();
      return false;
    }
  }
  return true;
}

auto ExpressionToDot::VisitLambdaExpr(LambdaExpr* expr) -> bool {
  ASSERT(expr);
  const auto node = NewNode();
  ASSERT(node);
  {
    // create node labels
    // label
    std::stringstream label;
    label << expr->GetName() << std::endl;
    SetNodeLabel(node, label);
  }
  CreateEdgeFromParent(node);
  {
    // process children
    NodeScope scope(this, node);
    if (!expr->VisitChildren(this)) {
      LOG(ERROR) << "failed to visit children of: " << expr->ToString();
      return false;
    }
  }
  return true;
}

auto ExpressionToDot::VisitQuotedExpr(QuotedExpr* expr) -> bool {
  ASSERT(expr);
  const auto node = NewNode();
  ASSERT(node);
  {
    // create node labels
    // label
    std::stringstream label;
    label << expr->GetName() << std::endl;
    SetNodeLabel(node, label);
  }
  CreateEdgeFromParent(node);
  return true;
}

auto ExpressionToDot::VisitSetExpr(SetExpr* expr) -> bool {
  ASSERT(expr);
  const auto node = NewNode();
  ASSERT(node);
  {
    // create node labels
    // label
    std::stringstream label;
    label << expr->GetName() << std::endl;
    label << "Symbol := " << expr->GetSymbol();
    SetNodeLabel(node, label);
  }
  CreateEdgeFromParent(node);
  {
    // process children
    NodeScope scope(this, node);
    if (!expr->VisitChildren(this)) {
      LOG(ERROR) << "failed to visit children of: " << expr->ToString();
      return false;
    }
  }
  return true;
}

auto ExpressionToDot::VisitThrowExpr(ThrowExpr* expr) -> bool {
  ASSERT(expr);
  const auto node = NewNode();
  ASSERT(node);
  {
    // create node labels
    // label
    std::stringstream label;
    label << expr->GetName() << std::endl;
    SetNodeLabel(node, label);
  }
  CreateEdgeFromParent(node);
  {
    // process children
    NodeScope scope(this, node);
    if (!expr->VisitChildren(this)) {
      LOG(ERROR) << "failed to visit children of: " << expr->ToString();
      return false;
    }
  }
  return true;
}

auto ExpressionToDot::VisitCondExpr(CondExpr* expr) -> bool {
  ASSERT(expr);
  const auto node = NewNode();
  ASSERT(node);
  {
    // create node labels
    // label
    std::stringstream label;
    label << expr->GetName() << std::endl;
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

auto ExpressionToDot::VisitModuleDef(ModuleDef* expr) -> bool {
  ASSERT(expr);
  const auto node = NewNode();
  ASSERT(node);
  {
    // create node labels
    // label
    std::stringstream label;
    label << expr->GetName() << std::endl;
    label << "Symbol := " << expr->GetSymbol()->Get();
    SetNodeLabel(node, label);
  }
  CreateEdgeFromParent(node);
  {
    // process children
    NodeScope scope(this, node);
    if (!expr->VisitChildren(this)) {
      LOG(ERROR) << "failed to visit children of: " << expr->ToString();
      return false;
    }
  }
  return true;
}

auto ExpressionToDot::VisitMacroDef(MacroDef* expr) -> bool {
  ASSERT(expr);
  const auto node = NewNode();
  ASSERT(node);
  {
    // create node labels
    // label
    std::stringstream label;
    label << expr->GetName() << std::endl;
    label << "Symbol := " << expr->GetSymbol()->Get();
    SetNodeLabel(node, label);
  }
  CreateEdgeFromParent(node);
  {
    // process children
    NodeScope scope(this, node);
    if (!expr->VisitChildren(this)) {
      LOG(ERROR) << "failed to visit children of: " << expr->ToString();
      return false;
    }
  }
  return true;
}

auto ExpressionToDot::VisitImportDef(ImportDef* expr) -> bool {
  ASSERT(expr);
  const auto node = NewNode();
  ASSERT(node);
  {
    // create node labels
    // label
    std::stringstream label;
    label << expr->GetName() << std::endl;
    label << "Symbol := " << expr->GetSymbol()->Get();
    SetNodeLabel(node, label);
  }
  CreateEdgeFromParent(node);
  return true;
}

auto ExpressionToDot::Build() -> dot::Graph* {
  return dot::Graph::New(this);
}
}  // namespace scm