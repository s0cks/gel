#include "scheme/ast_renderer.h"

#include <fmt/format.h>
#include <glog/logging.h>
#include <graphviz/cgraph.h>
#include <graphviz/gvc.h>

#include <cstdio>

#include "scheme/ast.h"
#include "scheme/gv.h"

namespace scm::ast {
auto GraphBuilder::CreateEdge(Agnode_t* from, Agnode_t* to, const char* name, const int flags) -> Agedge_t* {
  ASSERT(from);
  ASSERT(to);
  const auto edge = agedge(GetGraph(), from, to, const_cast<char*>(name), flags);
  ASSERT(edge);
  edges_.push_back(edge);
  return edge;
}

auto GraphBuilder::CreateNode(const char* name, const int flags) -> Agnode_t* {
  ASSERT(node);
  const auto node = agnode(GetGraph(), const_cast<char*>(name), flags);
  if (HasParent())
    CreateEdge(GetParent(), node, "");
  else if (HasPrevious())
    CreateEdge(GetPrevious(), node, "");
  return node;
}

auto GraphBuilder::VisitProgram(Program* p) -> bool {
  const auto node = CreateNode(p);
  SetParent(node);
  SetPrevious(node);
  return p->VisitAllForms(this);
}

auto GraphBuilder::VisitBeginDef(BeginDef* n) -> bool {
  const auto node = CreateNode("Begin");
  SetParent(node);
  return n->VisitChildren(this);
}

auto GraphBuilder::VisitBinaryOpExpr(BinaryOpExpr* p) -> bool {
  return true;
}

auto GraphBuilder::VisitBody(Body* p) -> bool {
  return true;
}

auto GraphBuilder::VisitVariableDef(VariableDef* n) -> bool {
  const auto node = NewVariableNode(n->GetVar());
  const auto old_parent = GetParent();
  SetParent(node);
  if (!n->GetVal()->Accept(this)) {
    LOG(ERROR) << "failed to visit: " << n->GetVal()->ToString();
    return false;
  }
  SetParent(old_parent);
  SetPrevious(node);
  return true;
}

auto GraphBuilder::VisitSyntaxDef(SyntaxDef* p) -> bool {
  return true;
}

auto GraphBuilder::VisitExpressionList(ExpressionList* p) -> bool {
  return true;
}

auto GraphBuilder::VisitQuoteExpr(QuoteExpr* p) -> bool {
  return true;
}

auto GraphBuilder::VisitCallProcExpr(CallProcExpr* p) -> bool {
  return true;
}

auto GraphBuilder::VisitConstantExpr(ConstantExpr* p) -> bool {
  const auto node = NewConstantNode(p->GetValue());
  ASSERT(node);
  SetPrevious(node);
  return true;
}

auto GraphBuilder::VisitLoadVariableExpr(LoadVariableExpr* p) -> bool {
  return true;
}

auto GraphBuilder::BuildDotGraph() -> DotGraph* {
  return DotGraph::New(GetGraph());
}

auto RenderToStdout(Node* node) -> bool {
  ASSERT(node);
  const auto graph = GraphBuilder::BuildDirected(node->GetName(), node);
  ASSERT(graph);
  DotGraphRenderer renderer;
  renderer.RenderDotToStdout(graph);
  return true;
}

auto RenderToPng(FILE* stream, Node* node) -> bool {
  ASSERT(stream);
  ASSERT(node);
  const auto graph = GraphBuilder::BuildDirected(node->GetName(), node);
  ASSERT(graph);
  DotGraphRenderer renderer;
  renderer.RenderPngTo(graph, stream);
  return true;
}
}  // namespace scm::ast