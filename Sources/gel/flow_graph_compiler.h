#ifndef GEL_FLOW_GRAPH_COMPILER_H
#define GEL_FLOW_GRAPH_COMPILER_H

#include <type_traits>

#include "gel/assembler.h"
#include "gel/common.h"
#include "gel/to_string_helper.h"
#include "gel/type_traits.h"

namespace gel {
template <class T>
concept HasSetCode = requires(T value) {
  { value.SetCode((CompiledCode*)nullptr) };
};

template <class T>
concept CompilerTarget = HasCompiledCode<T> && HasSetCode<T>;

class Lambda;
class Script;
class FlowGraph;
class Assembler;
class LocalScope;
class FlowGraphCompiler {
  DEFINE_NON_COPYABLE_TYPE(FlowGraphCompiler);

  struct BlockInfo {
   public:
    uword id = 0;
    Label label{};

    friend auto operator<<(std::ostream& stream, const BlockInfo& rhs) -> std::ostream& {
      stream << "BlockInfo(";
      stream << "id=" << rhs.id << ", ";
      stream << "label=" << rhs.label;
      stream << ")";
      return stream;
    }
  };

 private:
  LocalScope* scope_;
  Assembler assembler_{};
  std::vector<BlockInfo> info_{};

  template <CompilerTarget Target>
  auto BuildFlowGraph(Target& target) -> FlowGraph*;
  void AssembleFlowGraph(FlowGraph* graph);

 public:
  explicit FlowGraphCompiler(LocalScope* scope) :
    scope_(scope) {
    ASSERT(scope_);
    info_.emplace_back();
  }
  ~FlowGraphCompiler() = default;

  auto GetScope() const -> LocalScope* {
    return scope_;
  }

  auto assembler() -> Assembler* {
    return &assembler_;
  }

  auto GetBlockInfo(const uword idx) -> BlockInfo& {
    if (idx > info_.size())
      info_.resize(idx + 1);
    if (info_[idx].id != idx)
      info_[idx] = {
          .id = idx,
      };
    return info_[idx];
  }

  auto GetBlockLabel(const uword idx) -> Label* {
    return &GetBlockInfo(idx).label;
  }

  auto GetBlockInfo(ir::EntryInstr* blk) -> BlockInfo&;
  auto GetBlockLabel(ir::EntryInstr* blk) -> Label*;

  template <CompilerTarget Target>
  auto CompileTarget(Target& exec) -> bool;

 public:
  template <CompilerTarget Target>
  static inline auto Compile(Target& target, LocalScope* scope) -> bool {
    const auto& code = target.GetCode();
    if (code && code->IsCompiled())
      return true;
    ASSERT(scope);
    FlowGraphCompiler compiler(scope);
    return compiler.CompileTarget(target);
  }
};
}  // namespace gel

#endif  // GEL_FLOW_GRAPH_COMPILER_H
