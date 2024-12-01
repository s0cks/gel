#include "gel/flow_graph.h"

#include <fmt/format.h>
#include <glog/logging.h>

#include "gel/common.h"
#include "gel/instruction.h"

namespace gel {
auto FlowGraph::Accept(InstructionVisitor* vis) const -> bool {
  ASSERT(vis);
  InstructionIterator iter(GetEntry());
  while (iter.HasNext()) {
    const auto next = iter.Next();
    ASSERT(next);
    if (!next->Accept(vis))
      return false;
  }
  return true;
}
}  // namespace gel
