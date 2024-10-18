#ifndef SCM_AST_RENDERER_H
#define SCM_AST_RENDERER_H

#include <fmt/format.h>
#include <glog/logging.h>
#include <graphviz/cgraph.h>

#include "scheme/ast.h"
#include "scheme/common.h"
#include "scheme/gv.h"

namespace scm::ast {
class GraphBuilder : public DotGraphBuilder, protected NodeVisitor {
  DEFINE_NON_COPYABLE_TYPE(GraphBuilder);

 private:
  Agnode_t* root_ = nullptr;
  Agnode_t* parent_ = nullptr;
  Agnode_t* previous_ = nullptr;
  std::vector<Agedge_t*> edges_{};
  uint64_t num_vars_ = 0;
  uint64_t num_constants_ = 0;

  inline void SetParent(Agnode_t* node) {
    ASSERT(node);
    parent_ = node;
  }

  inline auto GetParent() const -> Agnode_t* {
    return parent_;
  }

  inline auto HasParent() const -> bool {
    return GetParent() != nullptr;
  }

  inline void SetPrevious(Agnode_t* node) {
    ASSERT(node);
    previous_ = node;
  }

  inline auto GetPrevious() const -> Agnode_t* {
    return previous_;
  }

  inline auto HasPrevious() const -> bool {
    return GetPrevious() != nullptr;
  }

  auto CreateEdge(Agnode_t* from, Agnode_t* to, const char* name, const int flags = 1) -> Agedge_t*;
  auto CreateNode(const char* name, const int flags = 1) -> Agnode_t*;

  inline auto CreateNode(const std::string& name, const int flags = 1) -> Agnode_t* {
    return CreateNode(name.c_str(), flags);
  }

  inline auto CreateNode(Node* node, const int flags = 1) -> Agnode_t* {
    ASSERT(node);
    return CreateNode(node->GetName(), flags);
  }

  inline auto NewConstantNode(Datum* val, const int flags = 1) -> Agnode_t* {
    const auto node_id = fmt::format("c{0:d}", num_constants_++);
    const auto node = CreateNode(node_id, flags);
    ASSERT(node);
    agset(node, "label", val->ToString().c_str());
    return node;
  }

  inline auto NewVariableNode(Variable* var, const int flags = 1) -> Agnode_t* {
    const auto node_id = fmt::format("var{0:d}", num_constants_++);
    const auto node = CreateNode(node_id, flags);
    ASSERT(node);
    agset(node, "label", var->GetName().c_str());
    return node;
  }

 public:
  GraphBuilder(const char* name, const Agdesc_t desc) :
    DotGraphBuilder(name, desc) {}
  ~GraphBuilder() override = default;

  auto BuildDotGraph() -> DotGraph* override;
#define DECLARE_VISIT(Name) auto Visit##Name(Name* node) -> bool override;
  FOR_EACH_AST_NODE(DECLARE_VISIT)
#undef DECLARE_VISIT
 public:
  static inline auto Build(const char* name, const Agdesc_t desc, Node* node) -> DotGraph* {
    GraphBuilder builder(name, desc);
    if (!node->Accept(&builder)) {
      LOG(ERROR) << "failed to visit: " << node->ToString();
      return nullptr;
    }
    return builder.BuildDotGraph();
  }

  static inline auto BuildDirected(const char* name, Node* node) -> DotGraph* {
    return Build(name, Agdirected, node);
  }
};

auto RenderToDot(FILE* stream, Node* node) -> bool;
auto RenderToPng(FILE* stream, Node* node) -> bool;
auto RenderToStdout(Node* node) -> bool;
}  // namespace scm::ast

#endif  // SCM_AST_RENDERER_H
