#ifndef SCM_INSTRUCTION_H
#define SCM_INSTRUCTION_H

#include <string>

#include "scheme/ast.h"
#include "scheme/common.h"
#include "scheme/procedure.h"
#include "scheme/variable.h"

namespace scm {
#define FOR_EACH_INSTRUCTION(V) \
  V(ConstantInstr)              \
  V(StoreVariableInstr)         \
  V(LoadVariableInstr)          \
  V(GraphEntryInstr)            \
  V(CallProcInstr)              \
  V(ReturnInstr)                \
  V(BinaryOpInstr)

#define FORWARD_DECLARE(Name) class Name;
FOR_EACH_INSTRUCTION(FORWARD_DECLARE)
#undef FORWARD_DECLARE

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

  virtual auto IsEntryInstr() const -> bool {
    return true;
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

#define DECLARE_INSTRUCTION(Name)                \
  DEFINE_NON_COPYABLE_TYPE(Name)                 \
 public:                                         \
  auto ToString() const -> std::string override; \
  auto GetName() const -> const char* override { \
    return #Name;                                \
  }                                              \
  auto As##Name() -> Name* override {            \
    return this;                                 \
  }

class EntryInstr : public Instruction {
  DEFINE_NON_COPYABLE_TYPE(EntryInstr);

 protected:
  EntryInstr() = default;

 public:
  ~EntryInstr() override = default;

  auto IsEntryInstr() const -> bool override {
    return true;
  }

  virtual auto GetFirstInstruction() const -> Instruction* = 0;
  auto GetLastInstruction() const -> Instruction*;
};

class GraphEntryInstr : public EntryInstr {
 private:
  GraphEntryInstr() = default;

 public:
  ~GraphEntryInstr() override = default;

  auto GetFirstInstruction() const -> Instruction* override;

  DECLARE_INSTRUCTION(GraphEntryInstr);

 public:
  static inline auto New() -> GraphEntryInstr* {
    return new GraphEntryInstr();
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
  Datum* value_;

 public:
  ConstantInstr(Datum* value) :
    Definition(),
    value_(value) {}
  ~ConstantInstr() override = default;

  auto GetValue() const -> Datum* {
    return value_;
  }

  DECLARE_INSTRUCTION(ConstantInstr);
};

class LoadVariableInstr : public Definition {
 private:
  Variable* var_;

 public:
  explicit LoadVariableInstr(Variable* var) :
    Definition(),
    var_(var) {
    ASSERT(var);
  }
  ~LoadVariableInstr() override = default;

  auto GetVariable() const -> Variable* {
    return var_;
  }

  DECLARE_INSTRUCTION(LoadVariableInstr);
};

class StoreVariableInstr : public Definition {
 private:
  Variable* var_;
  Definition* val_;

 public:
  StoreVariableInstr(Variable* var, Definition* val) :
    Definition(),
    var_(var),
    val_(val) {}
  ~StoreVariableInstr() override = default;

  auto GetVariable() const -> Variable* {
    return var_;
  }

  auto GetValue() const -> Definition* {
    return val_;
  }

  DECLARE_INSTRUCTION(StoreVariableInstr);
};

class CallProcInstr : public Instruction {
 private:
  Procedure* procedure_ = nullptr;
  Definition* args_ = nullptr;

 protected:
  explicit CallProcInstr(Procedure* proc, Definition* args) :
    Instruction() {
    SetProcedure(proc);
    SetArgs(args);
  }

  void SetProcedure(Procedure* proc) {
    ASSERT(proc);
    procedure_ = proc;
  }

  void SetArgs(Definition* args) {
    ASSERT(args);
    args_ = args;
  }

 public:
  ~CallProcInstr() override = default;

  auto GetProcedure() const -> Procedure* {
    return procedure_;
  }

  auto GetArgs() const -> Definition* {
    return args_;
  }

  DECLARE_INSTRUCTION(CallProcInstr);
};

class ReturnInstr : public Definition {
 private:
  Definition* value_;

 public:
  explicit ReturnInstr(Definition* value) :
    Definition(),
    value_(value) {}
  ~ReturnInstr() override = default;

  auto GetValue() const -> Definition* {
    return value_;
  }

  DECLARE_INSTRUCTION(ReturnInstr);
};

class BinaryOpInstr : public Definition {
 private:
  ast::BinaryOp op_;

 public:
  explicit BinaryOpInstr(const ast::BinaryOp op) :
    Definition(),
    op_(op) {}
  ~BinaryOpInstr() override = default;

  auto GetOp() const -> ast::BinaryOp {
    return op_;
  }

  DECLARE_INSTRUCTION(BinaryOpInstr);
};
}  // namespace scm

#endif  // SCM_INSTRUCTION_H
