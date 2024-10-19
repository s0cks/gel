#ifndef SCM_AST_DOT_H
#define SCM_AST_DOT_H

#include <fmt/format.h>

#include "scheme/ast.h"
#include "scheme/common.h"
#include "scheme/gv.h"

namespace scm::ast {
class NodeToDot : public DotGraphBuilder, protected NodeVisitor {
  DEFINE_NON_COPYABLE_TYPE(NodeToDot);

 private:
  Program* program_ = nullptr;
  Node* parent_ = nullptr;
  uint64_t num_nodes_ = 0;

  inline void SetProgram(Program* program) {
    ASSERT(program);
    program_ = program;
  }

  inline void SetParent(Node* node) {
    ASSERT(node);
    parent_ = node;
  }

  inline auto GetParent() const -> Node* {
    return parent_;
  }

  inline auto HasParent() const -> bool {
    return GetParent() != nullptr;
  }

  inline auto CreateNewNode(const int flags = kDefaultNodeFlags) -> Node* {
    return NewNode(fmt::format("n{0:d}", ++num_nodes_), flags);
  }

 public:
  NodeToDot(const char* graph_name, Program* program) :
    DotGraphBuilder(graph_name, Agdirected) {
    SetProgram(program);
  }
  ~NodeToDot() override = default;

  auto GetProgram() const -> Program* {
    return program_;
  }

  auto HasProgram() const -> bool {
    return GetProgram() != nullptr;
  }

  auto BuildDotGraph() -> DotGraph* override;
#define DEFINE_VISIT(Name) auto Visit##Name(Name* node) -> bool override;
  FOR_EACH_AST_NODE(DEFINE_VISIT)
#undef DEFINE_VISIT

 public:
  static inline auto Build(const char* graph_name, Program* program) -> DotGraph* {
    ASSERT(graph_name);
    ASSERT(program);
    NodeToDot builder(graph_name, program);
    const auto dot_graph = builder.BuildDotGraph();
    ASSERT(dot_graph);
    return dot_graph;
  }
};
}  // namespace scm::ast

#endif  // SCM_AST_DOT_H
