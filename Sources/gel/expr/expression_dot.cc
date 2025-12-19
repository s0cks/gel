#include "expr/expression_dot.h"
#ifdef GEL_ENABLE_GRAPHVIZ

#include <cstdio>
#include <filesystem>
#include <glog/logging.h>
#include <ostream>
#include <sstream>
#include <string>

#include "common.h"
#include "expr/exprs.h"
#include "gv.h"
#include "types.h"

namespace gel::expr {
ExpressionToDot::ExpressionToDot(const char* graph_name) :
  dot::GraphBuilder(graph_name) {
  dot::SetGraphNodeAttr(GetGraph(), "label", "");
  dot::SetGraphNodeAttr(GetGraph(), "xlabel", "");
  dot::SetGraphEdgeAttr(GetGraph(), "arrowhead", "vee");
  dot::SetGraphEdgeAttr(GetGraph(), "decorate", "true");
}

auto ExpressionToDot::VisitBindingExpr(BindingExpr* expr) -> bool {
  ASSERT(expr);
  const auto node = NewNode();
  ASSERT(node);
  {
    // create node labels
    // label
    std::stringstream label;
    label << expr->GetName() << std::endl;
    dot::SetNodeLabel(node, label);
  }
  CreateEdgeFromParent(node);
  return ProcessChildren(expr, node);
}

auto ExpressionToDot::VisitNewExpr(NewExpr* expr) -> bool {
  ASSERT(expr);
  // create new node
  const auto node = NewNode();
  ASSERT(node);
  {
    // create node labels
    // label
    std::stringstream label;
    label << expr->GetName() << std::endl;
    label << "Type: " << expr->GetTargetClass()->GetName();
    dot::SetNodeLabel(node, label);
  }
  CreateEdgeFromParent(node);
  return ProcessChildren(expr, node);
}

auto ExpressionToDot::VisitSeqExpr(SeqExpr* expr) -> bool {
  ASSERT(expr);
  // create new node
  const auto node = NewNode();
  ASSERT(node);
  {
    // create node labels
    // label
    std::stringstream label;
    label << expr->GetName() << std::endl;
    dot::SetNodeLabel(node, label);
    // xlabel
    std::stringstream xlabel;
    xlabel << expr->GetNumberOfChildren() << " expressions";
    dot::SetNodeXLabel(node, xlabel);
  }
  CreateEdgeFromParent(node);
  return ProcessChildren(expr, node);
}

auto ExpressionToDot::VisitImportExpr(ImportExpr* expr) -> bool {  // TODO: add import target
  ASSERT(expr);
  // create new node
  const auto node = NewNode();
  ASSERT(node);
  {
    // create node labels
    // label
    std::stringstream label;
    label << expr->GetName() << std::endl;
    dot::SetNodeLabel(node, label);
  }
  CreateEdgeFromParent(node);
  return ProcessChildren(expr, node);
}

auto ExpressionToDot::VisitNewMapExpr(NewMapExpr* expr) -> bool {
  ASSERT(expr);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return false;
}

auto ExpressionToDot::VisitStoreFieldExpr(StoreFieldExpr* expr) -> bool {
  ASSERT(expr);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return false;
}

auto ExpressionToDot::VisitStoreLocalExpr(StoreLocalExpr* expr) -> bool {
  ASSERT(expr);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return true;
}

auto ExpressionToDot::VisitInvokeMacroExpr(InvokeMacroExpr* expr) -> bool {
  ASSERT(expr);
  const auto node = NewNode();
  ASSERT(node);
  {
    // label
    std::stringstream label{};
    label << expr->GetName() << std::endl;
    label << "Target: " << expr->GetTarget()->GetSymbol()->GetFullyQualifiedName();
    dot::SetNodeLabel(node, label);
  }
  CreateEdgeFromParent(node);
  NodeScope scope(this, node);
  for (auto idx = 0; idx < expr->GetNumberOfArgs(); idx++) {
    const auto arg = expr->GetArgAt(idx);
    ASSERT(arg);
    if (!arg->Accept(*this)) {
      LOG(ERROR) << "failed to visit arg #" << idx << ": " << arg->ToString();
      return false;
    }
  }
  return true;
}

auto ExpressionToDot::VisitInvokeNativeExpr(InvokeNativeExpr* expr) -> bool {
  ASSERT(expr);
  // create new node
  const auto node = NewNode();
  ASSERT(node);
  {
    // label
    std::stringstream label{};
    label << expr->GetName() << std::endl;
    label << "Target: " << expr->GetTarget()->GetSymbol()->GetFullyQualifiedName();
    dot::SetNodeLabel(node, label);
  }
  CreateEdgeFromParent(node);
  return ProcessChildren(expr, node);
}

auto ExpressionToDot::VisitInvokeInstanceExpr(InvokeInstanceExpr* expr) -> bool {
  ASSERT(expr);
  // create new node
  const auto node = NewNode();
  {
    // label
    std::stringstream label{};
    label << expr->GetName() << std::endl;
    label << "Target: " << expr->GetTarget()->GetSymbol()->GetFullyQualifiedName();
    dot::SetNodeLabel(node, label);
  }
  CreateEdgeFromParent(node);
  return ProcessChildren(expr, node);
}

auto ExpressionToDot::VisitLoadInstanceMethodExpr(LoadInstanceMethodExpr* expr) -> bool {
  ASSERT(expr);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return false;
}

auto ExpressionToDot::VisitLoadFieldExpr(LoadFieldExpr* expr) -> bool {
  ASSERT(expr);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return false;
}

auto ExpressionToDot::VisitDoExpr(DoExpr* expr) -> bool {
  ASSERT(expr);
  // create new node
  const auto node = NewNode();
  ASSERT(node);
  {
    // create node labels
    // label
    std::stringstream label;
    label << expr->GetName() << std::endl;
    dot::SetNodeLabel(node, label);
    // xlabel
    std::stringstream xlabel;
    xlabel << expr->GetNumberOfChildren() << " expressions";
    dot::SetNodeXLabel(node, xlabel);
  }
  CreateEdgeFromParent(node);
  return ProcessChildren(expr, node);
}

auto ExpressionToDot::VisitClauseExpr(expr::ClauseExpr* expr) -> bool {
  ASSERT(expr);
  const auto node = NewNode();
  ASSERT(node);
  {
    // create node labels
    std::stringstream label;
    label << expr->GetName() << std::endl;
    dot::SetNodeLabel(node, label);
  }
  CreateEdgeFromParent(node);
  return ProcessChildren(expr, node);
}

auto ExpressionToDot::VisitWhileExpr(expr::WhileExpr* expr) -> bool {
  ASSERT(expr);
  const auto node = NewNode();
  ASSERT(node);
  {
    // create node labels
    std::stringstream label;
    label << expr->GetName() << std::endl;
    dot::SetNodeLabel(node, label);
  }
  CreateEdgeFromParent(node);
  return ProcessChildren(expr, node);
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
    dot::SetNodeLabel(node, label);
  }
  CreateEdgeFromParent(node);
  return ProcessChildren(expr, node);
}

auto ExpressionToDot::VisitInvokeExpr(InvokeExpr* expr) -> bool {
  ASSERT(expr);
  // create new node
  const auto node = NewNode();
  ASSERT(node);
  {
    std::stringstream label;
    label << expr->GetName() << std::endl;
    dot::SetNodeLabel(node, label);
  }
  CreateEdgeFromParent(node);
  {
    // target
    NodeScope scope(this, node);
    if (!expr->VisitTarget(*this)) {
      LOG(ERROR) << "failed to visit target: " << expr->GetTarget();
    }
  }
  {
    // args
    NodeScope scope(this, node);
    if (!expr->VisitArgs(*this)) {
      LOG(ERROR) << "failed to visit children of: " << expr->ToString();
      return false;
    }
  }
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
    label << expr->GetValue()->ToString();
    dot::SetNodeLabel(node, label);
  }
  CreateEdgeFromParent(node);
  return true;
}

auto ExpressionToDot::ProcessChildren(Expression* expr, dot::Node* node) -> bool {
  ASSERT(node);
  NodeScope scope(this, node);
  if (!expr->VisitChildren(*this)) {
    LOG(ERROR) << "failed to visit children of: " << expr->ToString();
    return false;
  }
  return true;
}

auto ExpressionToDot::VisitUnaryOpExpr(UnaryOpExpr* expr) -> bool {
  ASSERT(expr);
  const auto node = NewNode();
  ASSERT(node);
  {
    // create node labels
    // label
    std::stringstream label;
    label << expr->GetName() << std::endl;
    label << "Op := " << expr->GetOp();
    dot::SetNodeLabel(node, label);
  }
  CreateEdgeFromParent(node);
  return ProcessChildren(expr, node);
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
    dot::SetNodeLabel(node, label);
  }
  CreateEdgeFromParent(node);
  return ProcessChildren(expr, node);
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
    dot::SetNodeLabel(node, label);
  }
  CreateEdgeFromParent(node);
  return ProcessChildren(expr, node);
}

auto ExpressionToDot::VisitForeachExpr(expr::ForeachExpr* expr) -> bool {
  ASSERT(expr);
  const auto node = NewNode();
  ASSERT(node);
  {
    // create node labels
    // label
    std::stringstream label;
    label << expr->GetName() << std::endl;
    dot::SetNodeLabel(node, label);
  }
  CreateEdgeFromParent(node);
  return ProcessChildren(expr, node);
}

auto ExpressionToDot::VisitLetExpr(LetExpr* expr) -> bool {
  ASSERT(expr);
  const auto node = NewNode();
  ASSERT(node);
  {
    // create node labels
    // label
    std::stringstream label;
    label << expr->GetName() << std::endl;
    dot::SetNodeLabel(node, label);
  }
  CreateEdgeFromParent(node);
  return ProcessChildren(expr, node);
}

auto ExpressionToDot::Build() -> dot::DotGraph* {
  return dot::DotGraph::New(this);
}

void GenerateExprDotPng(const std::filesystem::path& path, const std::string& name, Expression* expr) {
  ASSERT(!name.empty());
  ASSERT(expr);
  dot::GraphRenderer render{};
  const auto dot = ExpressionToDot::BuildGraph(name, expr);
  ASSERT(dot);
  const auto file = fopen(path.c_str(), "wb");
  ASSERT(file);
  render.RenderPngTo(dot->get(), file);
  fclose(file);
}
}  // namespace gel::expr

#endif  // GEL_ENABLE_GRAPHVIZ
