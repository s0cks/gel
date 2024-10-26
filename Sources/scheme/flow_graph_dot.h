#ifndef SCM_FLOW_GRAPH_DOT_H
#define SCM_FLOW_GRAPH_DOT_H

#include <fmt/format.h>

#include <vector>

#include "scheme/flow_graph.h"
#include "scheme/instruction.h"

namespace scm {

class FlowGraphToDotGraph : public dot::GraphBuilder {
  DEFINE_NON_COPYABLE_TYPE(FlowGraphToDotGraph);

 private:
  uint64_t num_instructions_ = 0;
  const FlowGraph* flow_graph_;
  NodeList nodes_{};
  EdgeList edges_{};
  Node* previous_ = nullptr;

  inline void SetPrevious(Node* previous) {
    ASSERT(previous);
    previous_ = previous;
  }

  inline auto GetPrevious() const -> Node* {
    return previous_;
  }

  inline auto HasPrevious() const -> bool {
    return GetPrevious() != nullptr;
  }

  inline auto NewNode(Instruction* instr) -> Node* {
    ASSERT(instr);
    const auto node_id = fmt::format("i{0:d}", num_instructions_++);
    const auto node = GraphBuilder::NewNode(node_id);
    ASSERT(node);
    SetNodeLabel(node, instr->GetName());
    return node;
  }

 public:
  explicit FlowGraphToDotGraph(const char* name, const FlowGraph* flow_graph) :
    dot::GraphBuilder(name, Agdirected),
    flow_graph_(flow_graph) {
    ASSERT(flow_graph);
  }
  ~FlowGraphToDotGraph() override = default;

  auto GetFlowGraph() const -> const FlowGraph* {
    return flow_graph_;
  }

  auto HasFlowGraph() const -> bool {
    return GetFlowGraph() != nullptr;
  }

  auto GetGraphEntryInstr() const -> GraphEntryInstr* {
    return GetFlowGraph()->GetEntry();
  }

  auto Build() -> dot::Graph* override;

 public:
  static inline auto BuildGraph(const char* name, const FlowGraph* flow_graph) -> dot::Graph* {
    ASSERT(name);
    ASSERT(flow_graph);
    FlowGraphToDotGraph builder(name, flow_graph);
    const auto dot_graph = builder.Build();
    ASSERT(dot_graph);
    return dot_graph;
  }
};
}  // namespace scm

#endif  // SCM_FLOW_GRAPH_DOT_H
