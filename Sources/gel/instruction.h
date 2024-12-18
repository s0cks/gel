#ifndef GEL_INSTRUCTION_H
#define GEL_INSTRUCTION_H

#include <string>
#include <type_traits>

#include "gel/common.h"
#include "gel/expression.h"
#include "gel/lambda.h"
#include "gel/local.h"
#include "gel/procedure.h"
#include "gel/type_traits.h"
#include "gel/variable.h"

#define FOR_EACH_INSTRUCTION(V) \
  V(ConstantInstr)              \
  V(UnaryOpInstr)               \
  V(BinaryOpInstr)              \
  V(StoreLocalInstr)            \
  V(LoadLocalInstr)             \
  V(GraphEntryInstr)            \
  V(TargetEntryInstr)           \
  V(JoinEntryInstr)             \
  V(InvokeInstr)                \
  V(InvokeDynamicInstr)         \
  V(InvokeNativeInstr)          \
  V(ReturnInstr)                \
  V(BranchInstr)                \
  V(GotoInstr)                  \
  V(ThrowInstr)                 \
  V(InstanceOfInstr)            \
  V(CastInstr)                  \
  V(NewInstr)

namespace gel {
class EffectVisitor;
class ClauseVisitor;
class FlowGraphBuilder;

namespace ir {
class Definition;
class EntryInstr;
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
#define DECLARE_VISIT(Name) virtual auto Visit##Name(Name* instr)->bool = 0;
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

#define DEFINE_TYPE_CHECK(Name)    \
  virtual auto As##Name()->Name* { \
    return nullptr;                \
  }                                \
  auto Is##Name()->bool {          \
    return As##Name() != nullptr;  \
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
  friend class gel::EffectVisitor;
  friend class gel::ClauseVisitor;
  friend class gel::FlowGraphBuilder;
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

  auto GetLastInstruction() const -> Instruction*;
  auto VisitDominated(InstructionVisitor* vis) -> bool;
};

class GraphEntryInstr : public EntryInstr {
 private:
  explicit GraphEntryInstr(const uint64_t blk_id) :
    EntryInstr(blk_id) {}

 public:
  ~GraphEntryInstr() override = default;

  auto HasTarget() const -> bool {
    return HasNext() && GetNext()->IsTargetEntryInstr();
  }

  auto GetTarget() const -> TargetEntryInstr* {
    return HasTarget() ? GetNext()->AsTargetEntryInstr() : nullptr;
  }

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

  auto AsDefinition() -> Definition* override {
    return this;
  }

  auto IsDefinition() -> bool {
    return AsDefinition() != nullptr;
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
  explicit InvokeNativeInstr(Definition* target, const uint64_t num_args) :
    InvokeInstr(target, num_args) {}

 public:
  ~InvokeNativeInstr() override = default;

  DECLARE_INSTRUCTION(InvokeNativeInstr);

 public:
  static inline auto New(Definition* target, const uint64_t num_args = 0) -> InvokeNativeInstr* {
    ASSERT(target);
    ASSERT(num_args >= 0);
    return new InvokeNativeInstr(target, num_args);
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
 private:
  Definition* test_ = nullptr;
  EntryInstr* true_target_ = nullptr;
  EntryInstr* false_target_ = nullptr;
  JoinEntryInstr* join_ = nullptr;

 protected:
  explicit BranchInstr(Definition* test, EntryInstr* true_target, EntryInstr* false_target, JoinEntryInstr* join) :
    Instruction() {
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

  inline void SetTrueTarget(EntryInstr* target) {
    ASSERT(target);
    true_target_ = target;
  }

  inline void SetFalseTarget(EntryInstr* target) {
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

  DECLARE_INSTRUCTION(BranchInstr);

 public:
  static inline auto New(Definition* test, EntryInstr* true_target, EntryInstr* false_target, JoinEntryInstr* join)
      -> BranchInstr* {
    ASSERT(test);
    ASSERT(true_target);
    return new BranchInstr(test, true_target, false_target, join);
  }

  static inline auto New(Definition* test, EntryInstr* true_target, JoinEntryInstr* join) -> BranchInstr* {
    ASSERT(test);
    ASSERT(true_target);
    return New(test, true_target, nullptr, join);
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
}  // namespace ir

using ir::EntryInstr;
using ir::Instruction;
using ir::InstructionIterator;
using ir::InstructionVisitor;
#define DEFINE_USE(Name) using ir::Name;
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
