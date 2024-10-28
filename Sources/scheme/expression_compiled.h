#ifndef SCM_EXPRESSION_COMPILED_H
#define SCM_EXPRESSION_COMPILED_H

#include "scheme/flow_graph.h"

namespace scm {
class Runtime;
class FlowGraph;
class CompiledExpression : public Procedure {
 private:
  GraphEntryInstr* entry_ = nullptr;

 protected:
  explicit CompiledExpression(GraphEntryInstr* entry) :
    Procedure() {
    SetEntry(entry);
  }

  inline void SetEntry(GraphEntryInstr* entry) {
    ASSERT(entry);
    entry_ = entry;
  }

 public:
  auto GetEntry() const -> GraphEntryInstr* {
    return entry_;
  }

  inline auto HasEntry() const -> bool {
    return GetEntry() != nullptr;
  }

  auto Apply(Runtime* state) const -> bool override;
  DECLARE_TYPE(CompiledExpression);

 public:
  static inline auto New(GraphEntryInstr* entry) -> CompiledExpression* {
    ASSERT(entry);
    return new CompiledExpression(entry);
  }

  static inline auto New(FlowGraph* graph) -> CompiledExpression* {
    ASSERT(graph && graph->HasEntry());
    return New(graph->GetEntry());
  }
};
}  // namespace scm

#endif  // SCM_EXPRESSION_COMPILED_H
