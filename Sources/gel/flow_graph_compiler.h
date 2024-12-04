#ifndef GEL_FLOW_GRAPH_COMPILER_H
#define GEL_FLOW_GRAPH_COMPILER_H

#include "gel/common.h"

namespace gel {
class Lambda;
class FlowGraph;
class LocalScope;
class FlowGraphCompiler {
  DEFINE_NON_COPYABLE_TYPE(FlowGraphCompiler);

 private:
  LocalScope* scope_;

 public:
  explicit FlowGraphCompiler(LocalScope* scope) :
    scope_(scope) {
    ASSERT(scope_);
  }
  ~FlowGraphCompiler() = default;

  auto GetScope() const -> LocalScope* {
    return scope_;
  }

  auto CompileLambda(Lambda* lambda) -> bool;

 public:
  static auto Compile(Lambda* lambda, LocalScope* scope) -> bool;
};
}  // namespace gel

#endif  // GEL_FLOW_GRAPH_COMPILER_H
