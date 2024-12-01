#ifndef GEL_FLOW_GRAPH_H
#define GEL_FLOW_GRAPH_H

#include "gel/gv.h"
#include "gel/instruction.h"

namespace gel {
class FlowGraph {
  friend class FlowGraphBuilder;
  DEFINE_NON_COPYABLE_TYPE(FlowGraph);

 private:
  GraphEntryInstr* entry_;

  explicit FlowGraph(GraphEntryInstr* entry) :
    entry_(entry) {
    ASSERT(entry);
  }

 public:
  ~FlowGraph() = default;

  auto GetEntry() const -> GraphEntryInstr* {
    return entry_;
  }

  auto HasEntry() const -> bool {
    return GetEntry() != nullptr;
  }

  auto Accept(InstructionVisitor* vis) const -> bool;

 public:
  static inline auto New(GraphEntryInstr* entry) {
    ASSERT(entry);
    return new FlowGraph(entry);
  }
};
}  // namespace gel

#endif  // GEL_FLOW_GRAPH_H
