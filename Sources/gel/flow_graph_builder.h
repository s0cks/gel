#ifndef GEL_FLOW_GRAPH_BUILDER_H
#define GEL_FLOW_GRAPH_BUILDER_H

#include <type_traits>

#include "gel/common.h"
#include "gel/expression.h"
#include "gel/flow_graph.h"
#include "gel/instruction.h"
#include "gel/local.h"
#include "gel/object.h"
#include "gel/type_traits.h"

namespace gel {
class FlowGraphBuilder {
  friend class ClauseVisitor;
  friend class EffectVisitor;
  DEFINE_NON_COPYABLE_TYPE(FlowGraphBuilder);

 private:
  LocalScope* scope_ = nullptr;
  GraphEntryInstr* entry_ = nullptr;
  EntryInstr* block_ = nullptr;
  uint64_t num_blocks_ = 0;

  inline void SetScope(LocalScope* scope) {
    ASSERT(scope);
    scope_ = scope;
  }

  inline void SetCurrentBlock(EntryInstr* instr) {
    ASSERT(instr);
    block_ = instr;
  }

  inline auto GetCurrentBlock() const -> EntryInstr* {
    return block_;
  }

  inline void SetGraphEntry(GraphEntryInstr* instr) {
    ASSERT(instr);
    entry_ = instr;
  }

  inline auto GetNextBlockId() -> uint64_t {
    const auto next = num_blocks_;
    num_blocks_++;
    return next;
  }

  inline auto PushScope() -> LocalScope* {
    const auto new_scope = LocalScope::New(GetScope());
    SetScope(new_scope);
    return new_scope;
  }

  inline auto PushScope(const std::vector<LocalScope*>& scopes) -> LocalScope* {
    const auto new_scope = LocalScope::Union(scopes, GetScope());
    SetScope(new_scope);
    return new_scope;
  }

  inline void PopScope() {
    ASSERT(HasScope());
    SetScope(GetScope()->GetParent());
  }

 public:
  explicit FlowGraphBuilder(LocalScope* scope) :
    scope_(scope) {
    ASSERT(scope_);
  }
  ~FlowGraphBuilder() = default;

  auto GetScope() const -> LocalScope* {
    return scope_;
  }

  inline auto HasScope() const -> bool {
    return GetScope() != nullptr;
  }

  auto GetGraphEntry() const -> GraphEntryInstr* {
    return entry_;
  }

  auto HasGraphEntry() const -> bool {
    return GetGraphEntry() != nullptr;
  }

 public:
  static auto Build(Script* script, LocalScope* scope) -> FlowGraph*;
  static auto Build(Lambda* lambda, LocalScope* scope) -> FlowGraph*;
};

class ValueVisitor;
class EffectVisitor : public ExpressionVisitor {
  friend class FlowGraphBuilder;
  DEFINE_NON_COPYABLE_TYPE(EffectVisitor);

 private:
  FlowGraphBuilder* owner_;
  Instruction* entry_ = nullptr;
  Instruction* exit_ = nullptr;
  EntryInstr* block_ = nullptr;

 protected:
  virtual void Do(ir::Definition* defn) {
    ASSERT(defn);
    if (IsEmpty()) {
      SetEntryInstr(defn);
    } else {
      Instruction::Link(GetExitInstr(), defn);
    }
    SetExitInstr(defn);
  }

  virtual void ReturnDefinition(ir::Definition* defn) {
    ASSERT(defn);
    if (!defn->IsConstantInstr())
      Do(defn);
  }

  inline void SetEntryInstr(Instruction* instr) {
    entry_ = instr;
  }

  inline void SetExitInstr(Instruction* instr) {
    exit_ = instr;
  }

  inline void Add(Instruction* instr) {
    ASSERT(instr);
    if (IsEmpty()) {
      SetEntryInstr(instr);
      SetExitInstr(instr);
    } else {
      Instruction::Link(GetExitInstr(), instr);
      SetExitInstr(instr);
    }
  }

  void AddInstanceOf(ir::Definition* defn, Class* expected);
  auto CreateCallFor(ir::Definition* defn, const uword num_args) -> ir::InvokeInstr*;
  auto CreateStoreLoad(LocalVariable* local, ir::Definition* value) -> ir::Definition*;
  auto CreateCastTo(ir::Definition* value, Class* target) -> ir::Definition*;

  inline auto DoCastTo(ir::Definition* defn, Class* expected) -> ir::Definition* {
    ASSERT(defn);
    ASSERT(expected);
    const auto casted = ir::CastInstr::New(defn, expected);
    Do(casted);
    return casted;
  }

  inline void AddReturnExit(ir::Definition* value) {
    Add(ir::ReturnInstr::New(value));
    exit_ = nullptr;
  }

  auto ReturnCall(ir::InvokeInstr* defn) -> bool;
  auto ReturnCallTo(ir::Definition* defn, const uword num_args) -> bool;
  auto ReturnCallTo(Procedure* procedure, const uword num_args) -> bool;

  void Append(const EffectVisitor& rhs) {
    if (rhs.IsEmpty())
      return;
    if (IsEmpty()) {
      SetEntryInstr(rhs.GetEntryInstr());
    } else {
      Instruction::Link(GetExitInstr(), rhs.GetEntryInstr());
    }
    SetExitInstr(rhs.GetExitInstr());
  }

  auto Bind(ir::Definition* defn) -> ir::Definition* {
    if (IsEmpty()) {
      SetEntryInstr(defn);
    } else {
      Instruction::Link(GetExitInstr(), defn);
    }
    SetExitInstr(defn);
    return defn;
  }

  template <class E>
  inline auto NewBlock(std::enable_if_t<ir::is_entry<E>::value>* = nullptr) -> E* {
    const auto parent = GetCurrentBlock();
    const auto blk = E::New(GetOwner()->GetNextBlockId());
    if (parent)
      parent->AddDominated(blk);
    SetCurrentBlock(blk);
    return blk;
  }

  inline void SetCurrentBlock(EntryInstr* instr) {
    ASSERT(instr);
    block_ = instr;
  }

  inline auto GetCurrentBlock() const -> EntryInstr* {
    return block_;
  }

  inline auto CreateReturnForExit(ir::Instruction* exit_instr) -> ir::ReturnInstr* {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
    return exit_instr->IsDefinition() ? ir::ReturnInstr::New((ir::Definition*)exit_instr) : ir::ReturnInstr::New();
  }

  inline void AddImplicitReturn() {
    const auto exit = GetExitInstr();
    if (exit && !exit->IsReturnInstr())
      Add(CreateReturnForExit(exit));
  }

  virtual void ReturnValue(ir::Definition* defn) {}

 public:
  explicit EffectVisitor(FlowGraphBuilder* owner) :
    ExpressionVisitor(),
    owner_(owner) {}
  ~EffectVisitor() override = default;

  auto GetOwner() const -> FlowGraphBuilder* {
    return owner_;
  }

  auto GetEntryInstr() const -> Instruction* {
    return entry_;
  }

  auto GetExitInstr() const -> Instruction* {
    return exit_;
  }

  auto IsEmpty() const -> bool {
    return GetEntryInstr() == nullptr;
  }

  auto IsOpen() const -> bool {
    return IsEmpty() || GetExitInstr() != nullptr;
  }

  auto VisitScript(Script* script) -> bool;
  auto VisitLambda(Lambda* lambda) -> bool;
#define DECLARE_VISIT(Name) virtual auto Visit##Name(Name* name)->bool override;
  FOR_EACH_EXPRESSION_NODE(DECLARE_VISIT)
#undef DECLARE_VISIT
};

class ValueVisitor : public EffectVisitor {
  DEFINE_NON_COPYABLE_TYPE(ValueVisitor);

 private:
  ir::Definition* value_ = nullptr;

 protected:
  void ReturnValue(ir::Definition* value) override {
    ASSERT(value);
    value_ = value;
  }

  void ReturnDefinition(ir::Definition* defn) override {
    value_ = Bind(defn);
  }

 public:
  explicit ValueVisitor(FlowGraphBuilder* owner) :
    EffectVisitor(owner) {}
  ~ValueVisitor() override = default;

  auto GetValue() const -> ir::Definition* {
    return value_;
  }

  inline auto HasValue() const -> bool {
    return GetValue() != nullptr;
  }
};

class RxEffectVisitor : public EffectVisitor {
  DEFINE_NON_COPYABLE_TYPE(RxEffectVisitor);

 private:
  ir::Definition* observable_;

 public:
  RxEffectVisitor(FlowGraphBuilder* owner, ir::Definition* observable) :
    EffectVisitor(owner),
    observable_(observable) {
    ASSERT(observable_);
  }
  ~RxEffectVisitor() override = default;

  auto GetObservable() const -> ir::Definition* {
    return observable_;
  }

  auto VisitRxOpExpr(expr::RxOpExpr* expr) -> bool override;
};
}  // namespace gel

#endif  // GEL_FLOW_GRAPH_BUILDER_H
