#ifndef SCM_INSTRUCTION_H
#define SCM_INSTRUCTION_H

#include <string>

#include "scheme/common.h"
#include "scheme/expression.h"
#include "scheme/procedure.h"
#include "scheme/variable.h"

namespace scm {
class EffectVisitor;
class FlowGraphBuilder;
namespace instr {
#define FOR_EACH_INSTRUCTION(V) \
  V(ConstantInstr)              \
  V(UnaryOpInstr)               \
  V(BinaryOpInstr)              \
  V(ConsInstr)                  \
  V(StoreVariableInstr)         \
  V(LoadVariableInstr)          \
  V(GraphEntryInstr)            \
  V(TargetEntryInstr)           \
  V(JoinEntryInstr)             \
  V(InvokeInstr)                \
  V(InvokeNativeInstr)          \
  V(ReturnInstr)                \
  V(BranchInstr)                \
  V(GotoInstr)                  \
  V(ThrowInstr)                 \
  V(TypecheckInstr)

class Instruction;
#define FORWARD_DECLARE(Name) class Name;
FOR_EACH_INSTRUCTION(FORWARD_DECLARE)
#undef FORWARD_DECLARE

class InstructionVisitor {
  DEFINE_NON_COPYABLE_TYPE(InstructionVisitor);

 protected:
  InstructionVisitor() = default;

 public:
  virtual ~InstructionVisitor() = default;
#define DECLARE_VISIT(Name) virtual auto Visit##Name(Name* instr) -> bool = 0;
  FOR_EACH_INSTRUCTION(DECLARE_VISIT)
#undef DECLARE_VISIT
};

class Instruction {
  DEFINE_NON_COPYABLE_TYPE(Instruction);

 private:
  Instruction* next_ = nullptr;
  Instruction* previous_ = nullptr;

 protected:
  Instruction() = default;

  void SetNext(Instruction* instr) {
    ASSERT(instr);
    next_ = instr;
  }

  void SetPrevious(Instruction* instr) {
    ASSERT(instr);
    previous_ = instr;
  }

 public:
  virtual ~Instruction() = default;
  virtual auto GetName() const -> const char* = 0;
  virtual auto ToString() const -> std::string = 0;
  virtual auto Accept(InstructionVisitor* vis) -> bool = 0;

  auto GetNext() const -> Instruction* {
    return next_;
  }

  auto HasNext() const -> bool {
    return GetNext() != nullptr;
  }

  auto GetPrevious() const -> Instruction* {
    return previous_;
  }

  auto HasPrevious() const -> bool {
    return GetPrevious() != nullptr;
  }

  void Append(Instruction* instr);

  virtual auto IsEntryInstr() const -> bool {
    return false;
  }

  virtual auto IsDefinition() const -> bool {
    return false;
  }

#define DEFINE_TYPE_CHECK(Name)      \
  virtual auto As##Name() -> Name* { \
    return nullptr;                  \
  }                                  \
  auto Is##Name() -> bool {          \
    return As##Name() != nullptr;    \
  }
  FOR_EACH_INSTRUCTION(DEFINE_TYPE_CHECK)
#undef DEFINE_TYPE_CHECK

 public:
  static inline void Link(Instruction* lhs, Instruction* rhs) {
    ASSERT(lhs);
    lhs->SetNext(rhs);
    if (rhs)
      rhs->SetPrevious(lhs);
  }
};

class InstructionIterator {
  DEFINE_NON_COPYABLE_TYPE(InstructionIterator);

 private:
  Instruction* current_;

 public:
  explicit InstructionIterator(Instruction* start) :
    current_(start) {}
  ~InstructionIterator() = default;

  auto HasNext() const -> bool {
    return current_ != nullptr;
  }

  auto Next() -> Instruction* {
    const auto next = current_;
    current_ = next->GetNext();
    return next;
  }
};

#define DECLARE_INSTRUCTION(Name)                        \
  DEFINE_NON_COPYABLE_TYPE(Name)                         \
 public:                                                 \
  auto Accept(InstructionVisitor* vis) -> bool override; \
  auto ToString() const -> std::string override;         \
  auto GetName() const -> const char* override {         \
    return #Name;                                        \
  }                                                      \
  auto As##Name() -> Name* override {                    \
    return this;                                         \
  }

class EntryInstr : public Instruction {
  friend class scm::EffectVisitor;
  friend class scm::FlowGraphBuilder;
  DEFINE_NON_COPYABLE_TYPE(EntryInstr);

 private:
  uint64_t block_id_ = 0;
  EntryInstr* dominator_ = nullptr;
  std::vector<EntryInstr*> dominated_{};

 protected:
  explicit EntryInstr(const uint64_t blk_id) :
    block_id_(blk_id) {}

  inline void SetDominator(EntryInstr* instr) {
    ASSERT(instr);
    dominator_ = instr;
  }

  inline void AddDominated(EntryInstr* instr) {
    ASSERT(instr);
    instr->SetDominator(this);
    dominated_.push_back(instr);
  }

 public:
  ~EntryInstr() override = default;

  auto GetBlockId() const -> uint64_t {
    return block_id_;
  }

  auto IsEntryInstr() const -> bool override {
    return true;
  }

  auto GetDominator() const -> EntryInstr* {
    return dominator_;
  }

  auto HasDominator() const -> bool {
    return GetDominator() != nullptr;
  }

  auto GetNumberOfDominatedBlocks() const -> uint64_t {
    return dominated_.size();
  }

  auto GetDominatedBlockAt(const uint64_t idx) const -> EntryInstr* {
    ASSERT(idx >= 0 && idx <= GetNumberOfDominatedBlocks());
    return dominated_[idx];
  }

  virtual auto GetFirstInstruction() const -> Instruction* {
    return GetNext();
  }

  auto GetLastInstruction() const -> Instruction*;
  auto VisitDominated(InstructionVisitor* vis) -> bool;
};

class GraphEntryInstr : public EntryInstr {
 private:
  explicit GraphEntryInstr(const uint64_t blk_id) :
    EntryInstr(blk_id) {}

 public:
  ~GraphEntryInstr() override = default;

  auto GetFirstInstruction() const -> Instruction* override;

  DECLARE_INSTRUCTION(GraphEntryInstr);

 public:
  static inline auto New(const uint64_t blk_id) -> GraphEntryInstr* {
    return new GraphEntryInstr(blk_id);
  }
};

class TargetEntryInstr : public EntryInstr {
 private:
  explicit TargetEntryInstr(const uint64_t blk_id) :
    EntryInstr(blk_id) {}

 public:
  ~TargetEntryInstr() override = default;
  DECLARE_INSTRUCTION(TargetEntryInstr);

 public:
  static inline auto New(const uint64_t blk_id) -> TargetEntryInstr* {
    return new TargetEntryInstr(blk_id);
  }
};

class JoinEntryInstr : public EntryInstr {
 private:
  explicit JoinEntryInstr(const uint64_t blk_id) :
    EntryInstr(blk_id) {}

 public:
  ~JoinEntryInstr() override = default;
  DECLARE_INSTRUCTION(JoinEntryInstr);

 public:
  static inline auto New(const uint64_t blk_id) -> JoinEntryInstr* {
    return new JoinEntryInstr(blk_id);
  }
};

class Definition : public Instruction {
  DEFINE_NON_COPYABLE_TYPE(Definition);

 protected:
  Definition() = default;

 public:
  ~Definition() override = default;

  auto IsDefinition() const -> bool override {
    return true;
  }
};

class ConstantInstr : public Definition {
 private:
  Type* value_;

  explicit ConstantInstr(Type* value) :
    Definition(),
    value_(value) {}

 public:
  ~ConstantInstr() override = default;

  auto GetValue() const -> Type* {
    return value_;
  }

  DECLARE_INSTRUCTION(ConstantInstr);

 public:
  static inline auto New(Type* value) -> ConstantInstr* {
    ASSERT(value);
    return new ConstantInstr(value);
  }
};

class LoadVariableInstr : public Definition {
 private:
  Symbol* symbol_ = nullptr;

  inline void SetSymbol(Symbol* symbol) {
    ASSERT(symbol);
    symbol_ = symbol;
  }

 public:
  explicit LoadVariableInstr(Symbol* symbol) :
    Definition() {
    SetSymbol(symbol);
  }
  ~LoadVariableInstr() override = default;

  auto GetSymbol() const -> Symbol* {
    return symbol_;
  }

  DECLARE_INSTRUCTION(LoadVariableInstr);

 public:
  static inline auto New(Symbol* symbol) -> LoadVariableInstr* {
    ASSERT(symbol);
    return new LoadVariableInstr(symbol);
  }
};

class StoreVariableInstr : public Instruction {
 private:
  Symbol* symbol_;
  Definition* value_;

  StoreVariableInstr(Symbol* symbol, Definition* value) :
    Instruction(),
    symbol_(symbol),
    value_(value) {}

 public:
  ~StoreVariableInstr() override = default;

  auto GetSymbol() const -> Symbol* {
    return symbol_;
  }

  auto GetValue() const -> Definition* {
    return value_;
  }

  DECLARE_INSTRUCTION(StoreVariableInstr);

 public:
  static inline auto New(Symbol* symbol, Definition* value) -> StoreVariableInstr* {
    ASSERT(symbol);
    ASSERT(value);
    return new StoreVariableInstr(symbol, value);
  }
};

class ThrowInstr : public Instruction {
 private:
  Definition* value_;

 protected:
  explicit ThrowInstr(Definition* value) :
    Instruction(),
    value_(value) {}

 public:
  ~ThrowInstr() override = default;

  auto GetValue() const -> Definition* {
    return value_;
  }

  DECLARE_INSTRUCTION(ThrowInstr);

 public:
  static inline auto New(Definition* defn) -> ThrowInstr* {
    ASSERT(defn);
    return new ThrowInstr(defn);
  }
};

class InvokeInstr : public Definition {
 private:
  Definition* target_;

 protected:
  explicit InvokeInstr(Definition* target) :
    Definition(),
    target_(target) {
    ASSERT(target_);
  }

 public:
  ~InvokeInstr() override = default;

  auto GetTarget() const -> Definition* {
    return target_;
  }

  DECLARE_INSTRUCTION(InvokeInstr);

 public:
  static inline auto New(Definition* target) -> InvokeInstr* {
    ASSERT(target);
    return new InvokeInstr(target);
  }
};

class InvokeNativeInstr : public Definition {
 private:
  Definition* target_;

 protected:
  explicit InvokeNativeInstr(Definition* target) :
    Definition(),
    target_(target) {
    ASSERT(target_);
  }

 public:
  ~InvokeNativeInstr() override = default;

  auto GetTarget() const -> Definition* {
    return target_;
  }

  DECLARE_INSTRUCTION(InvokeNativeInstr);

 public:
  static inline auto New(Definition* target) -> InvokeNativeInstr* {
    ASSERT(target);
    return new InvokeNativeInstr(target);
  }
};

class ReturnInstr : public Definition {
 private:
  Definition* value_;

 protected:
  explicit ReturnInstr(Definition* value) :
    Definition(),
    value_(value) {}

 public:
  ~ReturnInstr() override = default;

  auto GetValue() const -> Definition* {
    return value_;
  }

  inline auto HasValue() const -> bool {
    return GetValue() != nullptr;
  }

  DECLARE_INSTRUCTION(ReturnInstr);

 public:
  static inline auto New(Definition* value = nullptr) -> ReturnInstr* {
    return new ReturnInstr(value);
  }
};

class BinaryOpInstr : public Definition {
 private:
  expr::BinaryOp op_;

  explicit BinaryOpInstr(const expr::BinaryOp op) :
    Definition(),
    op_(op) {}

 public:
  ~BinaryOpInstr() override = default;

  auto GetOp() const -> expr::BinaryOp {
    return op_;
  }

  DECLARE_INSTRUCTION(BinaryOpInstr);

 public:
  static inline auto New(const expr::BinaryOp op) -> BinaryOpInstr* {
    return new BinaryOpInstr(op);
  }
};

class UnaryOpInstr : public Definition {
 private:
  expr::UnaryOp op_;
  Definition* value_;

  explicit UnaryOpInstr(const expr::UnaryOp op, Definition* value) :
    Definition(),
    op_(op),
    value_(value) {
    ASSERT(value_);
  }

 public:
  ~UnaryOpInstr() override = default;

  auto GetOp() const -> expr::UnaryOp {
    return op_;
  }

  auto GetValue() const -> Definition* {
    return value_;
  }

  DECLARE_INSTRUCTION(UnaryOpInstr);

 public:
  static inline auto New(const expr::UnaryOp op, Definition* value) -> UnaryOpInstr* {
    return new UnaryOpInstr(op, value);
  }
};

class BranchInstr : public Definition {
 private:
  Definition* test_ = nullptr;
  TargetEntryInstr* true_target_ = nullptr;
  TargetEntryInstr* false_target_ = nullptr;
  JoinEntryInstr* join_ = nullptr;

 protected:
  explicit BranchInstr(Definition* test, TargetEntryInstr* true_target, TargetEntryInstr* false_target, JoinEntryInstr* join) :
    Definition() {
    SetTest(test);
    SetTrueTarget(true_target);
    if (false_target)
      SetFalseTarget(false_target);
    SetJoin(join);
  }

  inline void SetTest(Definition* test) {
    ASSERT(test);
    test_ = test;
  }

  inline void SetTrueTarget(TargetEntryInstr* target) {
    ASSERT(target);
    true_target_ = target;
  }

  inline void SetFalseTarget(TargetEntryInstr* target) {
    ASSERT(target);
    false_target_ = target;
  }

  inline void SetJoin(JoinEntryInstr* join) {
    ASSERT(join);
    join_ = join;
  }

 public:
  ~BranchInstr() override = default;

  auto GetTest() const -> Definition* {
    return test_;
  }

  auto GetTrueTarget() const -> TargetEntryInstr* {
    return true_target_;
  }

  auto GetFalseTarget() const -> TargetEntryInstr* {
    return false_target_;
  }

  auto HasFalseTarget() const -> bool {
    return GetFalseTarget() != nullptr;
  }

  auto GetJoin() const -> JoinEntryInstr* {
    return join_;
  }

  auto HasJoin() const -> bool {
    return GetJoin() != nullptr;
  }

  DECLARE_INSTRUCTION(BranchInstr);

 public:
  static inline auto New(Definition* test, TargetEntryInstr* true_target, TargetEntryInstr* false_target = nullptr,
                         JoinEntryInstr* join = nullptr) -> BranchInstr* {
    ASSERT(test);
    ASSERT(true_target);
    return new BranchInstr(test, true_target, false_target, join);
  }
};

class GotoInstr : public Definition {
 private:
  EntryInstr* target_ = nullptr;

 protected:
  explicit GotoInstr(EntryInstr* target) :
    Definition() {
    SetTarget(target);
  }

  inline void SetTarget(EntryInstr* target) {
    ASSERT(target);
    target_ = target;
  }

 public:
  ~GotoInstr() override = default;

  auto GetTarget() const -> EntryInstr* {
    return target_;
  }

  auto HasTarget() const -> bool {
    return GetTarget() != nullptr;
  }

  DECLARE_INSTRUCTION(GotoInstr);

 public:
  static inline auto New(EntryInstr* target) -> GotoInstr* {
    ASSERT(target);
    return new GotoInstr(target);
  }
};

class ConsInstr : public Definition {
 private:
  Definition* car_;
  Definition* cdr_;

 protected:
  ConsInstr(Definition* car, Definition* cdr) :
    Definition(),
    car_(car),
    cdr_(cdr) {}

 public:
  ~ConsInstr() override = default;

  auto GetCar() const -> Definition* {
    return car_;
  }

  auto GetCdr() const -> Definition* {
    return cdr_;
  }

  DECLARE_INSTRUCTION(ConsInstr);

 public:
  static inline auto New(Definition* car, Definition* cdr) -> ConsInstr* {
    ASSERT(car);
    ASSERT(cdr);
    return new ConsInstr(car, cdr);
  }
};

class TypecheckInstr : public Instruction {
 private:
  Definition* value_;

 public:
  explicit TypecheckInstr(Definition* value) :
    Instruction(),
    value_(value) {
    ASSERT(value_);
  }
  ~TypecheckInstr() override = default;

  auto GetValue() const -> Definition* {
    return value_;
  }

  DECLARE_INSTRUCTION(TypecheckInstr);

 public:
  static inline auto New(Definition* value) -> TypecheckInstr* {
    ASSERT(value);
    return new TypecheckInstr(value);
  }
};
}  // namespace instr

using instr::EntryInstr;
using instr::Instruction;
using instr::InstructionIterator;
using instr::InstructionVisitor;
#define DEFINE_USE(Name) using instr::Name;
FOR_EACH_INSTRUCTION(DEFINE_USE)
#undef DEFINE_USE
}  // namespace scm

#endif  // SCM_INSTRUCTION_H
