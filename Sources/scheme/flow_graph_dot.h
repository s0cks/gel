#ifndef SCM_FLOW_GRAPH_DOT_H
#define SCM_FLOW_GRAPH_DOT_H

#include <fmt/format.h>

#include <vector>

#include "scheme/flow_graph.h"
#include "scheme/instruction.h"

namespace scm {

class FlowGraphToDotGraph : public DotGraphBuilder {
  DEFINE_NON_COPYABLE_TYPE(FlowGraphToDotGraph);

 private:
  uint64_t num_instructions_ = 0;
  const FlowGraph* flow_graph_;
  NodeList nodes_{};
  EdgeList edges_{};

  uint64_t num_constants_ = 0;
  Node* graph_entry_ = nullptr;
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

  inline auto NewNode(Instruction* instr, const int flags = kDefaultNodeFlags) -> Node* {
    ASSERT(HasGraph());
    ASSERT(instr);
    const auto node_id = fmt::format("i{0:d}", ++num_instructions_);
    return DotGraphBuilder::NewNode(node_id, flags);
  }

  inline auto NewEdgeFromPrevious(Agnode_t* to, const char* name = "", const int flags = kDefaultEdgeFlags) -> Edge* {
    ASSERT(HasPrevious());
    return NewEdge(GetPrevious(), to, name, flags);
  }

  static inline auto GetNodeNameFor(Instruction* instr) -> std::string {
    return instr->ToString();
  }

  void Set(Node* node, const std::string& name, const std::string& value);

  inline void SetLabel(Node* node, const std::string& value) {
    return Set(node, "label", value);
  }

  inline void SetXLabel(Node* node, const std::string& value) {
    return Set(node, "xlabel", value);
  }

  auto CreateXLabel(Instruction* instr) -> std::optional<std::string>;
  auto CreateNextNode(Instruction* instr) -> Node*;
  void InitNodes();
  void InitEdges();

 public:
  explicit FlowGraphToDotGraph(const char* name, const FlowGraph* flow_graph) :
    DotGraphBuilder(name, Agdirected),
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

  auto BuildDotGraph() -> DotGraph* override;

 public:
  static inline auto Build(const char* name, const FlowGraph* flow_graph) -> DotGraph* {
    ASSERT(name);
    ASSERT(flow_graph);
    FlowGraphToDotGraph builder(name, flow_graph);
    const auto dot_graph = builder.BuildDotGraph();
    ASSERT(dot_graph);
    return dot_graph;
  }
};
}  // namespace scm

#endif  // SCM_FLOW_GRAPH_DOT_H
