#ifndef GEL_INSTRUCTION_H
#define GEL_INSTRUCTION_H

#include <string>
#include <type_traits>

#include "gel/bitvector.h"
#include "gel/common.h"
#include "gel/expression.h"
#include "gel/lambda.h"
#include "gel/local.h"
#include "gel/procedure.h"
#include "gel/type_traits.h"
#include "gel/variable.h"

#define FOR_EACH_INSTRUCTION(V) \
  V(Constant)                   \
  V(UnaryOp)                    \
  V(BinaryOp)                   \
  V(StoreLocal)                 \
  V(LoadLocal)                  \
  V(GraphEntry)                 \
  V(TargetEntry)                \
  V(LetEntry)                   \
  V(JoinEntry)                  \
  V(LoadField)                  \
  V(StoreField)                 \
  V(Lookup)                     \
  V(Invoke)                     \
  V(InvokeDynamic)              \
  V(InvokeNative)               \
  V(Return)                     \
  V(Branch)                     \
  V(Goto)                       \
  V(Throw)                      \
  V(InstanceOf)                 \
  V(Cast)                       \
  V(New)                        \
  V(NewList)                    \
  V(Phi)

namespace gel {
class Assembler;
class EffectVisitor;
class NativeProcedure;
class FlowGraphBuilder;
class FlowGraphCompiler;

namespace ir {
class Definition;
class EntryInstr;
class Instruction;
#define FORWARD_DECLARE(Name) class Name##Instr;
FOR_EACH_INSTRUCTION(FORWARD_DECLARE)
#undef FORWARD_DECLARE

class InstructionVisitor {
  DEFINE_NON_COPYABLE_TYPE(InstructionVisitor);

 protected:
  InstructionVisitor() = default;

 public:
  virtual ~InstructionVisitor() = default;
#define DECLARE_VISIT(Name) virtual auto Visit##Name##Instr(Name##Instr* instr)->bool = 0;
  FOR_EACH_INSTRUCTION(DECLARE_VISIT)
#undef DECLARE_VISIT
};

class Instruction {
  friend class BranchInstr;
  friend class gel::FlowGraphCompiler;
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

  virtual void Compile(FlowGraphCompiler*) = 0;

 public:
  virtual ~Instruction() = default;
  virtual auto GetName() const -> const char* = 0;
  virtual auto ToString() const -> std::string = 0;
  virtual auto Accept(InstructionVisitor* vis) -> bool = 0;

  virtual auto GetSuccessorCount() const -> uword {
    return 0;
  }

  virtual auto GetSuccessorAt(const uword idx) const -> EntryInstr* {
    return nullptr;
  }

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

  virtual auto AsEntryInstr() -> EntryInstr* {
    return nullptr;
  }

  auto IsEntryInstr() -> bool {
    return AsEntryInstr() != nullptr;
  }

  virtual auto AsDefinition() -> Definition* {
    return nullptr;
  }

  auto IsDefinition() -> bool {
    return AsDefinition() != nullptr;
  }

#define DEFINE_TYPE_CHECK(Name)                  \
  virtual auto As##Name##Instr()->Name##Instr* { \
    return nullptr;                              \
  }                                              \
  auto Is##Name##Instr()->bool {                 \
    return As##Name##Instr() != nullptr;         \
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
  template <class E>
  explicit InstructionIterator(E* executable, std::enable_if_t<gel::is_executable<E>::value>* = nullptr) :
    current_(nullptr) {
    ASSERT(executable);
    current_ = executable->GetEntry();
    ASSERT(current_);
  }
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
 protected:                                              \
  void Compile(FlowGraphCompiler* assembler) override;   \
                                                         \
 public:                                                 \
  auto Accept(InstructionVisitor* vis) -> bool override; \
  auto ToString() const -> std::string override;         \
  auto GetName() const -> const char* override {         \
    return #Name;                                        \
  }                                                      \
  auto As##Name()->Name* override {                      \
    return this;                                         \
  }

class EntryInstr : public Instruction {
  friend class gel::FlowGraph;
  friend class gel::EffectVisitor;
  friend class gel::FlowGraphBuilder;
  DEFINE_NON_COPYABLE_TYPE(EntryInstr);

 private:
  word preorder_num_ = -1;
  word postorder_num_ = -1;
  uword offset_ = 0;
  uword start_pos_ = 0;
  uword end_pos_ = 0;
  Instruction* last_ = nullptr;

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

  void SetLastInstruction(Instruction* rhs) {
    ASSERT(rhs);
    last_ = rhs;
  }

  auto DiscoverBlocks(EntryInstr* predecessor, std::vector<EntryInstr*>& preorder, std::vector<word>& parent) -> bool;

 public:
  ~EntryInstr() override = default;

  void SetStartPos(const uword rhs) {
    start_pos_ = rhs;
  }

  auto GetStartPos() const -> uword {
    return start_pos_;
  }

  void SetEndPos(const uword rhs) {
    end_pos_ = rhs;
  }

  auto GetEndPos() const -> uword {
    return end_pos_;
  }

  auto GetPreorderNum() const -> word {
    return preorder_num_;
  }

  void SetPreorderNum(const word rhs) {
    preorder_num_ = rhs;
  }

  auto GetPostorderNum() const -> word {
    return postorder_num_;
  }

  void SetPostorderNum(const word rhs) {
    postorder_num_ = rhs;
  }

  auto GetBlockId() const -> uint64_t {
    return block_id_;
  }

  auto AsEntryInstr() -> EntryInstr* override {
    return this;
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

  auto GetLastInstruction() const -> Instruction* {
    return last_;
  }

  virtual void ClearDominated() {
    dominated_.clear();
  }

  auto VisitDominated(InstructionVisitor* vis) -> bool;
  virtual auto GetNumberOfPredecessors() const -> uword = 0;
  virtual auto GetPredecessorAt(const uword idx) const -> EntryInstr* = 0;
  virtual void ClearPredecessors() = 0;
  virtual void AddPredecessor(EntryInstr* predecessor) = 0;
};

class GraphEntryInstr : public EntryInstr {
 private:
  Object* procedure_;
  TargetEntryInstr* target_;

  explicit GraphEntryInstr(Object* procedure, const uint64_t blk_id, TargetEntryInstr* target) :
    EntryInstr(blk_id),
    procedure_(procedure),
    target_(target) {
    ASSERT(procedure_);
    ASSERT(target_);
  }

 public:
  ~GraphEntryInstr() override = default;

  auto GetProcedure() const -> Object* {
    return procedure_;
  }

  auto GetTarget() const -> TargetEntryInstr* {
    return target_;
  }

  auto GetFirstInstruction() const -> Instruction* override;

  void ClearPredecessors() override {}
  void AddPredecessor(EntryInstr* blk) override {}

  auto GetNumberOfPredecessors() const -> uword override {
    return 0;
  }

  auto GetPredecessorAt(const uword idx) const -> EntryInstr* override {
    return nullptr;
  }

  auto GetSuccessorCount() const -> uword override {
    return 1;
  }

  auto GetSuccessorAt(const uword idx) const -> EntryInstr* override;

  DECLARE_INSTRUCTION(GraphEntryInstr);

 public:
  static inline auto New(Object* proc, const uint64_t idx, TargetEntryInstr* target) -> GraphEntryInstr* {
    ASSERT(proc);
    return new GraphEntryInstr(proc, idx, target);
  }
};

class LetEntryInstr : public EntryInstr {
 private:
  EntryInstr* predecessor_ = nullptr;

  explicit LetEntryInstr(const uint64_t id) :
    EntryInstr(id) {}

 public:
  ~LetEntryInstr() override = default;

  void AddPredecessor(EntryInstr* blk) override {
    ASSERT(blk && predecessor_ == nullptr);
    predecessor_ = blk;
  }

  auto GetPredecessorAt(const uword idx) const -> EntryInstr* override {
    ASSERT(idx == 0 && predecessor_ != nullptr);
    return predecessor_;
  }

  void ClearPredecessors() override {
    predecessor_ = nullptr;
  }

  auto GetNumberOfPredecessors() const -> uword override {
    return 1;
  }

  DECLARE_INSTRUCTION(LetEntryInstr);

 public:
  static inline auto New(const uint64_t idx) -> LetEntryInstr* {
    return new LetEntryInstr(idx);
  }
};

class TargetEntryInstr : public EntryInstr {
 private:
  EntryInstr* predecessor_ = nullptr;

  explicit TargetEntryInstr(const uint64_t blk_id) :
    EntryInstr(blk_id) {}

 public:
  ~TargetEntryInstr() override = default;

  void AddPredecessor(EntryInstr* blk) override {
    ASSERT(blk && predecessor_ == nullptr);
    predecessor_ = blk;
  }

  auto GetPredecessorAt(const uword idx) const -> EntryInstr* override {
    ASSERT(idx == 0 && predecessor_ != nullptr);
    return predecessor_;
  }

  void ClearPredecessors() override {
    predecessor_ = nullptr;
  }

  auto GetNumberOfPredecessors() const -> uword override {
    return 1;
  }

  DECLARE_INSTRUCTION(TargetEntryInstr);

 public:
  static inline auto New(const uint64_t blk_id) -> TargetEntryInstr* {
    return new TargetEntryInstr(blk_id);
  }
};

class JoinEntryInstr : public EntryInstr {
 private:
  std::vector<EntryInstr*> predecessors_{};

  explicit JoinEntryInstr(const uint64_t blk_id) :
    EntryInstr(blk_id) {}

 public:
  ~JoinEntryInstr() override = default;

  auto IsLetJoin() const -> bool {
    for (const auto& predecessor : predecessors_) {
      if (predecessor->IsLetEntryInstr())
        return true;
    }
    return false;
  }

  void AddPredecessor(EntryInstr* blk) override {
    ASSERT(blk);
    predecessors_.push_back(blk);
  }

  auto GetPredecessorAt(const uword idx) const -> EntryInstr* override {
    return predecessors_[idx];
  }

  void ClearPredecessors() override {
    predecessors_.clear();
  }

  auto GetNumberOfPredecessors() const -> uword override {
    return predecessors_.size();
  }

  DECLARE_INSTRUCTION(JoinEntryInstr);

 public:
  static inline auto New(const uint64_t blk_id) -> JoinEntryInstr* {
    return new JoinEntryInstr(blk_id);
  }
};

class Input {
  DEFINE_NON_COPYABLE_TYPE(Input);

 private:
  Input* next_ = nullptr;
  Input* previous_ = nullptr;
  Definition* defn_;
  Instruction* instr_ = nullptr;
  word index_ = 0;

 public:
  explicit Input(Definition* defn) :
    defn_(defn) {
    ASSERT(defn_);
  }
  ~Input() = default;

  void SetNext(Input* rhs) {
    ASSERT(rhs);
    next_ = rhs;
  }

  auto GetNext() const -> Input* {
    return next_;
  }

  inline auto HasNext() const -> bool {
    return GetNext() != nullptr;
  }

  void SetPrevious(Input* rhs) {
    ASSERT(rhs);
    previous_ = rhs;
  }

  auto GetPrevious() const -> Input* {
    return previous_;
  }

  inline auto HasPrevious() const -> bool {
    return GetPrevious() != nullptr;
  }

  auto GetIndex() const -> word {
    return index_;
  }

  void SetIndex(const word rhs) {
    index_ = rhs;
  }

  auto GetInstruction() const -> Instruction* {
    return instr_;
  }

  void SetInstruction(Instruction* rhs) {
    ASSERT(rhs);
    instr_ = rhs;
  }

  auto GetDefinition() const -> Definition* {
    return defn_;
  }

  void SetDefinition(Definition* rhs) {
    ASSERT(rhs);
    defn_ = rhs;
  }

  void RemoveFromList();
  void Bind(Definition* rhs);

 public:
  static inline void Prepend(Input** list, Input* value) {
    const auto next = (*list);
    (*list) = value;
    if (next) {
      value->SetNext(next);
      next->SetPrevious(value);
    }
  }
};

class Definition : public Instruction {
  friend class Input;
  DEFINE_NON_COPYABLE_TYPE(Definition);

 private:
  Input* input_use_list_ = nullptr;

 protected:
  Definition() = default;

 public:
  ~Definition() override = default;

  auto AsDefinition() -> Definition* override {
    return this;
  }

  auto IsDefinition() -> bool {
    return AsDefinition() != nullptr;
  }

  void SetInputUseList(Input* rhs) {
    input_use_list_ = rhs;
  }

  auto GetInputUseList() const -> Input* {
    return input_use_list_;
  }

  void AddInput(Input* rhs) {
    ASSERT(rhs);
    Input::Prepend(&input_use_list_, rhs);
  }
};

class ConstantInstr : public Definition {
 private:
  Object* value_;

  explicit ConstantInstr(Object* value) :
    Definition(),
    value_(value) {}

 public:
  ~ConstantInstr() override = default;

  auto GetValue() const -> Object* {
    return value_;
  }

  inline auto IsConstantSymbol() const -> bool {
    return GetValue()->IsSymbol();
  }

  DECLARE_INSTRUCTION(ConstantInstr);

 public:
  static inline auto New(Object* value) -> ConstantInstr* {
    ASSERT(value);
    return new ConstantInstr(value);
  }
};

class LoadLocalInstr : public Definition {
 private:
  LocalVariable* local_;

 public:
  explicit LoadLocalInstr(LocalVariable* local) :
    Definition(),
    local_(local) {
    ASSERT(local_);
  }
  ~LoadLocalInstr() override = default;

  auto GetLocal() const -> LocalVariable* {
    return local_;
  }

  DECLARE_INSTRUCTION(LoadLocalInstr);

 public:
  static inline auto New(LocalVariable* local) -> LoadLocalInstr* {
    ASSERT(local);
    return new LoadLocalInstr(local);
  }
};

class StoreLocalInstr : public Instruction {
 private:
  LocalVariable* local_;
  Definition* value_;

  StoreLocalInstr(LocalVariable* local, Definition* value) :
    Instruction(),
    local_(local),
    value_(value) {
    ASSERT(local_);
    ASSERT(value_);
  }

 public:
  ~StoreLocalInstr() override = default;

  auto GetLocal() const -> LocalVariable* {
    return local_;
  }

  auto GetValue() const -> Definition* {
    return value_;
  }

  DECLARE_INSTRUCTION(StoreLocalInstr);

 public:
  static inline auto New(LocalVariable* local, Definition* value) -> StoreLocalInstr* {
    ASSERT(local);
    ASSERT(value);
    return new StoreLocalInstr(local, value);
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

class TemplateInvokeInstr : public Definition {
  DEFINE_NON_COPYABLE_TYPE(TemplateInvokeInstr);

 protected:
  TemplateInvokeInstr() = default;

 public:
  ~TemplateInvokeInstr() override = default;
};

class InvokeInstr : public Definition {
 private:
  Definition* target_;
  uint64_t num_args_;

 protected:
  explicit InvokeInstr(Definition* target, const uint64_t num_args) :
    Definition(),
    target_(target),
    num_args_(num_args) {
    ASSERT(target_);
  }

 public:
  ~InvokeInstr() override = default;

  auto GetNumberOfArgs() const -> uint64_t {
    return num_args_;
  }

  auto GetTarget() const -> Definition* {
    return target_;
  }

  auto GetProcedure() const -> Procedure* {
    ASSERT(GetTarget()->IsConstantInstr() && GetTarget()->AsConstantInstr()->GetValue()->IsProcedure());
    return GetTarget()->AsConstantInstr()->GetValue()->AsProcedure();
  }

  DECLARE_INSTRUCTION(InvokeInstr);

 public:
  static inline auto New(Definition* target, const uint64_t num_args) -> InvokeInstr* {
    ASSERT(target);
    return new InvokeInstr(target, num_args);
  }
};

class InvokeDynamicInstr : public Definition {
 private:
  Definition* target_;
  uint64_t num_args_;

 protected:
  explicit InvokeDynamicInstr(Definition* target, uint64_t num_args) :
    Definition(),
    target_(target),
    num_args_(num_args) {
    ASSERT(target_);
    ASSERT(num_args_ >= 0);
  }

 public:
  ~InvokeDynamicInstr() override = default;

  auto GetTarget() const -> Definition* {
    return target_;
  }

  auto GetNumberOfArgs() const -> uint64_t {
    return num_args_;
  }

  DECLARE_INSTRUCTION(InvokeDynamicInstr);

 public:
  static inline auto New(Definition* target, const uint64_t num_args) -> InvokeDynamicInstr* {
    ASSERT(target);
    ASSERT(num_args >= 0);
    return new InvokeDynamicInstr(target, num_args);
  }
};
class InvokeNativeInstr : public InvokeInstr {
 protected:
  explicit InvokeNativeInstr(Definition* target, const uword num_args) :
    InvokeInstr(target, num_args) {}

 public:
  ~InvokeNativeInstr() override = default;

  auto GetNativeProcedure() const -> NativeProcedure* {
    ASSERT(GetProcedure()->IsNativeProcedure());
    return GetProcedure()->AsNativeProcedure();
  }

  DECLARE_INSTRUCTION(InvokeNativeInstr);

 public:
  static inline auto New(Definition* target, const uword num_args) -> InvokeNativeInstr* {
    ASSERT(target);
    return new InvokeNativeInstr(target, num_args);
  }
};

class LookupInstr : public Definition {
 private:
  Definition* symbol_;

  explicit LookupInstr(Definition* symbol) :
    Definition(),
    symbol_(symbol) {
    ASSERT(symbol_);
  }

 public:
  ~LookupInstr() override = default;

  auto GetSymbol() const -> Definition* {
    return symbol_;
  }

  DECLARE_INSTRUCTION(LookupInstr);

 public:
  static inline auto New(Definition* defn) -> LookupInstr* {
    ASSERT(defn);
    return new LookupInstr(defn);
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

template <class Op>
class TemplateOpInstr : public Definition {
  DEFINE_NON_COPYABLE_TYPE(TemplateOpInstr);

 private:
  Op op_;

 protected:
  explicit TemplateOpInstr(const Op op) :
    Definition(),
    op_(op) {}

  void SetOp(const Op op) {
    op_ = op;
  }

 public:
  ~TemplateOpInstr() override = default;

  auto GetOp() const -> Op {
    return op_;
  }
};

class BinaryOpInstr : public TemplateOpInstr<expr::BinaryOp> {
 private:
  Definition* left_ = nullptr;
  Definition* right_ = nullptr;

 protected:
  explicit BinaryOpInstr(const expr::BinaryOp op, Definition* left, Definition* right) :
    TemplateOpInstr<expr::BinaryOp>(op) {
    SetLeft(left);
    SetRight(right);
  }

  void SetLeft(Definition* defn) {
    ASSERT(defn);
    left_ = defn;
  }

  void SetRight(Definition* defn) {
    ASSERT(defn);
    right_ = defn;
  }

 public:
  ~BinaryOpInstr() override = default;

  auto GetLeft() const -> Definition* {
    return left_;
  }

  auto GetRight() const -> Definition* {
    return right_;
  }

#define DEFINE_OP_CHECK(Name)                  \
  inline auto Is##Name##Op() const->bool {     \
    return GetOp() == expr::BinaryOp::k##Name; \
  }
  FOR_EACH_BINARY_OP(DEFINE_OP_CHECK)
#undef DEFINE_OP_CHECK

  DECLARE_INSTRUCTION(BinaryOpInstr);

 public:
  static inline auto New(const expr::BinaryOp op, Definition* left, Definition* right) -> BinaryOpInstr* {
    return new BinaryOpInstr(op, left, right);
  }

#define DEFINE_NEW_OP(Name)                                                           \
  static inline auto New##Name(Definition* left, Definition* right)->BinaryOpInstr* { \
    return New(expr::BinaryOp::k##Name, left, right);                                 \
  }
  FOR_EACH_BINARY_OP(DEFINE_NEW_OP)
#undef DEFINE_NEW_OP
};

class UnaryOpInstr : public TemplateOpInstr<expr::UnaryOp> {
 private:
  Definition* value_ = nullptr;

 protected:
  explicit UnaryOpInstr(const expr::UnaryOp op, Definition* value) :
    TemplateOpInstr<expr::UnaryOp>(op) {
    SetValue(value);
  }

  inline void SetValue(Definition* value) {
    ASSERT(value);
    value_ = value;
  }

 public:
  ~UnaryOpInstr() override = default;

  auto GetValue() const -> Definition* {
    return value_;
  }

#define DEFINE_OP_CHECK(Name)                 \
  inline auto Is##Name##Op() const->bool {    \
    return GetOp() == expr::UnaryOp::k##Name; \
  }
  FOR_EACH_UNARY_OP(DEFINE_OP_CHECK)
#undef DEFINE_OP_CHECK

  DECLARE_INSTRUCTION(UnaryOpInstr);

 public:
  static inline auto New(const expr::UnaryOp op, Definition* value) -> UnaryOpInstr* {
    return new UnaryOpInstr(op, value);
  }

#define DEFINE_NEW_OP(Name)                                        \
  static inline auto New##Name(Definition* value)->UnaryOpInstr* { \
    return New(expr::UnaryOp::k##Name, value);                     \
  }
  FOR_EACH_UNARY_OP(DEFINE_NEW_OP)
#undef DEFINE_NEW_OP
};

class BranchInstr : public Instruction {
 public:
  enum Condition : uint8_t {
    kTrue,
    kNotTrue,
    kEqual,
    kNotEqual,
  };

 private:
  Condition condition_;
  EntryInstr* true_target_;
  EntryInstr* false_target_;
  JoinEntryInstr* join_;

 protected:
  explicit BranchInstr(Condition condition, EntryInstr* true_target, EntryInstr* false_target, JoinEntryInstr* join) :
    Instruction(),
    condition_(condition),
    true_target_(true_target),
    false_target_(false_target),
    join_(join) {
    ASSERT(true_target_);
    ASSERT(join_);
  }

 public:
  ~BranchInstr() override = default;

  auto GetTrueTarget() const -> EntryInstr* {
    return true_target_;
  }

  auto GetFalseTarget() const -> EntryInstr* {
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

  auto GetSuccessorCount() const -> uword override {
    return HasFalseTarget() ? 2 : 1;
  }

  auto GetSuccessorAt(const uword idx) const -> EntryInstr* override {
    if (idx == 0)
      return GetTrueTarget();
    else if (idx == 1)
      return GetFalseTarget();
    NOT_IMPLEMENTED(ERROR);
    return nullptr;
  }

  DECLARE_INSTRUCTION(BranchInstr);

 public:
  static inline auto New(Condition condition, EntryInstr* true_target, EntryInstr* false_target, JoinEntryInstr* join)
      -> BranchInstr* {
    ASSERT(true_target);
    return new BranchInstr(condition, true_target, false_target, join);
  }

  static inline auto New(Condition condition, EntryInstr* true_target, JoinEntryInstr* join) -> BranchInstr* {
    ASSERT(true_target);
    return New(condition, true_target, nullptr, join);
  }

  static inline auto BranchTrue(EntryInstr* true_target, EntryInstr* false_target, JoinEntryInstr* join) -> BranchInstr* {
    ASSERT(true_target);
    ASSERT(join);
    return New(Condition::kTrue, true_target, false_target, join);
  }

  static inline auto BranchTrue(EntryInstr* true_target, JoinEntryInstr* join) -> BranchInstr* {
    ASSERT(true_target);
    ASSERT(join);
    return BranchTrue(true_target, nullptr, join);
  }

  static inline auto BranchFalse(EntryInstr* true_target, EntryInstr* false_target, JoinEntryInstr* join) -> BranchInstr* {
    ASSERT(true_target);
    ASSERT(join);
    return New(Condition::kNotTrue, true_target, false_target, join);
  }

  static inline auto BranchFalse(EntryInstr* true_target, JoinEntryInstr* join) -> BranchInstr* {
    ASSERT(true_target);
    ASSERT(join);
    return New(Condition::kNotTrue, true_target, nullptr, join);
  }

  static inline auto BranchEqual(EntryInstr* true_target, EntryInstr* false_target, JoinEntryInstr* join) -> BranchInstr* {
    ASSERT(true_target);
    ASSERT(join);
    return New(Condition::kEqual, true_target, false_target, join);
  }

  static inline auto BranchNotEqual(EntryInstr* true_target, EntryInstr* false_target, JoinEntryInstr* join) -> BranchInstr* {
    ASSERT(true_target);
    ASSERT(join);
    return New(Condition::kNotEqual, true_target, false_target, join);
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

  auto GetSuccessorCount() const -> uword override {
    return 1;
  }

  auto GetSuccessorAt(const uword idx) const -> EntryInstr* override {
    return GetTarget();
  }

  DECLARE_INSTRUCTION(GotoInstr);

 public:
  static inline auto New(EntryInstr* target) -> GotoInstr* {
    ASSERT(target);
    return new GotoInstr(target);
  }
};

class InstanceOfInstr : public Definition {
 public:
  using Predicate = std::function<bool(Object*)>;

 private:
  Definition* value_;
  Class* type_;
  bool strict_;

  explicit InstanceOfInstr(Definition* value, Class* type, const bool strict) :
    Definition(),
    value_(value),
    type_(type),
    strict_(strict) {
    ASSERT(value_);
    ASSERT(type_);
  }

 public:
  ~InstanceOfInstr() override = default;

  auto GetValue() const -> Definition* {
    return value_;
  }

  auto GetType() const -> Class* {
    return type_;
  }

  auto IsStrict() const -> bool {
    return strict_;
  }

  DECLARE_INSTRUCTION(InstanceOfInstr);

 public:
  static inline auto New(Definition* value, Class* type, const bool strict = true) -> InstanceOfInstr* {
    ASSERT(value);
    ASSERT(type);
    return new InstanceOfInstr(value, type, strict);
  }

  static inline auto NewStrict(Definition* value, Class* cls) -> InstanceOfInstr* {
    ASSERT(value);
    ASSERT(cls);
    return New(value, cls, true);
  }
};

class CastInstr : public Definition {
 private:
  Definition* value_;
  Class* target_;

 protected:
  CastInstr(Definition* value, Class* target) :
    Definition(),
    value_(value),
    target_(target) {
    ASSERT(value_);
    ASSERT(target_);
  }

 public:
  ~CastInstr() override = default;

  auto GetValue() const -> Definition* {
    return value_;
  }

  auto GetTarget() const -> Class* {
    return target_;
  }

  DECLARE_INSTRUCTION(CastInstr);

 public:
  static inline auto New(Definition* value, Class* cls) -> CastInstr* {
    ASSERT(value);
    ASSERT(cls);
    return new CastInstr(value, cls);
  }
};

class NewInstr : public Definition {
 private:
  Class* target_;
  uword num_args_;

 protected:
  NewInstr(Class* target, const uword num_args) :
    target_(target),
    num_args_(num_args) {
    ASSERT(target_);
  }

 public:
  ~NewInstr() override = default;

  auto GetTarget() const -> Class* {
    return target_;
  }

  auto GetNumberOfArgs() const -> uword {
    return num_args_;
  }

  DECLARE_INSTRUCTION(NewInstr);

 public:
  static inline auto New(Class* target, const uword num_args) -> NewInstr* {
    ASSERT(target);
    return new NewInstr(target, num_args);
  }
};

class NewListInstr : public Definition {
 private:
  uword length_;

 protected:
  NewListInstr(const uword length) :
    Definition(),
    length_(length) {}

 public:
  ~NewListInstr() override = default;

  auto GetLength() const -> uword {
    return length_;
  }

  DECLARE_INSTRUCTION(NewListInstr);

 public:
  static inline auto New(const uword length) -> NewListInstr* {
    return new NewListInstr(length);
  }
};

class LoadFieldInstr : public Definition {
 private:
  Definition* instance_;
  Field* field_;

  LoadFieldInstr(Definition* instance, Field* field) :
    Definition(),
    instance_(instance),
    field_(field) {
    ASSERT(instance_);
    ASSERT(field_);
  }

 public:
  ~LoadFieldInstr() override = default;

  auto GetInstance() const -> Definition* {
    return instance_;
  }

  auto GetField() const -> Field* {
    return field_;
  }

  DECLARE_INSTRUCTION(LoadFieldInstr);

 public:
  static inline auto New(Definition* instance, Field* field) -> LoadFieldInstr* {
    ASSERT(instance);
    ASSERT(field);
    return new LoadFieldInstr(instance, field);
  }
};

class StoreFieldInstr : public Definition {
 private:
  Field* field_;
  Definition* instance_;
  Definition* value_;

  StoreFieldInstr(Field* field, Definition* instance, Definition* value) :
    Definition(),
    field_(field),
    instance_(instance),
    value_(value) {
    ASSERT(instance_);
    ASSERT(field_);
    ASSERT(value_);
  }

 public:
  ~StoreFieldInstr() override = default;

  auto GetField() const -> Field* {
    return field_;
  }

  auto GetInstance() const -> Definition* {
    return instance_;
  }

  auto GetValue() const -> Definition* {
    return value_;
  }

  DECLARE_INSTRUCTION(StoreFieldInstr);

 public:
  static inline auto New(Field* field, Definition* instance, Definition* value) -> StoreFieldInstr* {
    ASSERT(field);
    ASSERT(instance);
    ASSERT(value);
    return new StoreFieldInstr(field, instance, value);
  }
};

class PhiInstr : public Definition {
 private:
  JoinEntryInstr* join_;
  word num_inputs_;
  BitVector* reaching_ = nullptr;
  bool alive_ = false;

  PhiInstr(JoinEntryInstr* join, const word num_inputs) :
    Definition(),
    join_(join),
    num_inputs_(num_inputs) {
    ASSERT(join_);
  }

 public:
  ~PhiInstr() override = default;

  DECLARE_INSTRUCTION(PhiInstr);

 public:
  static inline auto New(JoinEntryInstr* join, const word num_inputs) -> PhiInstr* {
    return new PhiInstr(join, num_inputs);
  }
};

static inline void PrintInstructions(Instruction* start) {
  ASSERT(start);
  InstructionIterator iter(start);
  while (iter.HasNext()) {
    const auto next = iter.Next();
    DLOG(INFO) << " - " << next->ToString();
  }
}
}  // namespace ir

using ir::EntryInstr;
using ir::Instruction;
using ir::InstructionIterator;
using ir::InstructionVisitor;
#define DEFINE_USE(Name) using ir::Name##Instr;
FOR_EACH_INSTRUCTION(DEFINE_USE)
#undef DEFINE_USE

#ifdef GEL_DEBUG

using Severity = google::LogSeverity;
class InstructionLogger {
  DEFINE_NON_COPYABLE_TYPE(InstructionLogger);

 private:
  Severity severity_;

 public:
  explicit InstructionLogger(const Severity severity) :
    severity_(severity) {}
  ~InstructionLogger() = default;

  auto GetSeverity() const -> Severity {
    return severity_;
  }

  inline void Visit(ir::Instruction* instr) {
    ASSERT(instr);
    LOG_AT_LEVEL(GetSeverity()) << " - " << instr->ToString();
  }

  inline void operator()(ir::Instruction* instr) {
    return Visit(instr);
  }

 public:
  template <const Severity S = google::INFO, const bool OnlyOne = false>
  static inline void Log(ir::Instruction* instr) {
    ASSERT(instr);
    InstructionLogger logger(S);
    logger.Visit(instr);
    if (OnlyOne)
      return;
    InstructionIterator iter(instr->GetNext());
    while (iter.HasNext()) {
      logger(iter.Next());
    }
  }

  template <class E, const Severity S = google::INFO, const bool OnlyOne = false>
  static inline void Log(E* executable, std::enable_if_t<gel::is_executable<E>::value>* = nullptr) {
    ASSERT(executable);
    return Log<S, OnlyOne>(executable->GetEntry());
  }
};

#endif  // GEL_DEBUG
}  // namespace gel

#endif  // GEL_INSTRUCTION_H
