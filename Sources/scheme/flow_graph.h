#ifndef SCM_FLOW_GRAPH_H
#define SCM_FLOW_GRAPH_H

#include "scheme/ast.h"
#include "scheme/common.h"
#include "scheme/gv.h"
#include "scheme/instruction.h"

namespace scm {
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

 public:
  static inline auto New(GraphEntryInstr* entry) {
    ASSERT(entry);
    return new FlowGraph(entry);
  }
};
}  // namespace scm

#endif  // SCM_FLOW_GRAPH_H
