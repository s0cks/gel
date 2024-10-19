#ifndef SCM_AST_H
#define SCM_AST_H

#include <ostream>
#include <utility>

#include "scheme/common.h"
#include "scheme/variable.h"

namespace scm {
class Parser;
}

namespace scm::ast {
#define FOR_EACH_AST_NODE(V) \
  V(Program)                 \
  V(BeginDef)                \
  V(SyntaxDef)               \
  V(VariableDef)             \
  V(ConstantExpr)            \
  V(QuoteExpr)               \
  V(LoadVariableExpr)        \
  V(Body)                    \
  V(CallProcExpr)            \
  V(ExpressionList)          \
  V(BinaryOpExpr)

class Node;
class Definition;
class Expression;
#define FORWARD_DECLARE(Name) class Name;
FOR_EACH_AST_NODE(FORWARD_DECLARE)

class NodeVisitor {  // NOLINT
 protected:
  NodeVisitor() = default;

 public:
  virtual ~NodeVisitor() = default;
#define DEFINE_VISIT(Name) virtual auto Visit##Name(Name* node) -> bool = 0;
  FOR_EACH_AST_NODE(DEFINE_VISIT)
#undef DEFINE_VISIT
};

class Node {
  DEFINE_NON_COPYABLE_TYPE(Node);

 protected:
  Node() = default;

 public:
  virtual ~Node() = default;

  virtual auto IsDefinition() const -> bool {
    return false;
  }

  virtual auto IsExpression() const -> bool {
    return false;
  }

  virtual auto IsConstantExpr() const -> bool {
    return false;
  }

  virtual auto ToString() const -> std::string = 0;
  virtual auto GetName() const -> const char* = 0;
  virtual auto Accept(NodeVisitor* vis) -> bool = 0;

#define DEFINE_TYPE_CHECK(Name)      \
  virtual auto As##Name() -> Name* { \
    return nullptr;                  \
  }                                  \
  auto Is##Name() -> bool {          \
    return As##Name() != nullptr;    \
  }
  FOR_EACH_AST_NODE(DEFINE_TYPE_CHECK)
#undef DEFINE_TYPE_CHECK
};

#define DECLARE_NODE_TYPE(Name)                   \
  DEFINE_NON_COPYABLE_TYPE(Name)                  \
 public:                                          \
  auto Accept(NodeVisitor* vis) -> bool override; \
  auto ToString() const -> std::string override;  \
  auto GetName() const -> const char* override {  \
    return #Name;                                 \
  }

class Form : public Node {
  DEFINE_NON_COPYABLE_TYPE(Form);

 protected:
  Form() = default;

 public:
  ~Form() override = default;
};

using FormList = std::vector<Form*>;

static inline auto operator<<(std::ostream& stream, const FormList& rhs) -> std::ostream& {
  stream << "[";
  auto remaining = rhs.size();
  for (const auto& form : rhs) {
    stream << form->ToString();
    if (--remaining >= 1)
      stream << ",";
  }
  stream << "]";
  return stream;
}

class DefinitionVisitor;
class Program : public Node {
  friend class scm::Parser;

 private:
  FormList forms_;

  explicit Program(const FormList& forms) :
    Node(),
    forms_(forms) {}

  void Append(Form* form) {
    ASSERT(form);
    forms_.push_back(form);
  }

 public:
  ~Program() override = default;

  auto GetForms() const -> const FormList& {
    return forms_;
  }

  auto IsEmpty() const -> bool {
    return GetForms().empty();
  }

  auto GetTotalNumberOfForms() const -> uint64_t {
    return forms_.size();
  }

  auto GetFormAt(const uint64_t idx) const -> Form* {
    ASSERT(idx >= 0 && idx <= GetTotalNumberOfForms());
    return forms_[idx];
  }

  auto VisitAllForms(NodeVisitor* vis) -> bool;
  auto VisitAllExpressions(NodeVisitor* vis) -> bool;
  auto VisitAllDefinitions(NodeVisitor* vis) -> bool;

  DECLARE_NODE_TYPE(Program);

 public:
  static inline auto New(const FormList& forms = {}) -> Program* {
    return new Program(forms);
  }
};

class Definition : public Form {
  DEFINE_NON_COPYABLE_TYPE(Definition);

 protected:
  Definition() = default;

 public:
  ~Definition() override = default;

  auto IsDefinition() const -> bool override {
    return true;
  }
};

using DefinitionList = std::vector<Definition*>;

static inline auto operator<<(std::ostream& stream, const DefinitionList& rhs) -> std::ostream& {
  stream << "[";
  auto remaining = rhs.size();
  for (const auto& defn : rhs) {
    stream << defn->ToString();
    if (remaining-- > 0)
      stream << ", ";
  }
  stream << "]";
  return stream;
}

class Value;
class Expression;
class VariableDef : public Definition {
 private:
  Variable* var_;
  Value* val_;

  VariableDef(Variable* var, Value* value) :
    Definition(),
    var_(var),
    val_(value) {
    ASSERT(var_);
    ASSERT(val_);
  }

 public:
  ~VariableDef() override = default;

  auto GetVar() const -> Variable* {
    return var_;
  }

  auto GetVal() const -> Value* {
    return val_;
  }

  DECLARE_NODE_TYPE(VariableDef);

 public:
  static inline auto New(Variable* var, Value* value) -> VariableDef* {
    ASSERT(var);
    ASSERT(value);
    return new VariableDef(var, value);
  }
};

class SyntaxDef : public Definition {
 private:
  std::string keyword_;

  explicit SyntaxDef(const std::string& keyword) :
    Definition(),
    keyword_(keyword) {}

 public:
  ~SyntaxDef() override = default;

  auto GetKeyword() const -> const std::string& {
    return keyword_;
  }

  DECLARE_NODE_TYPE(SyntaxDef);

 public:
  static inline auto New(const std::string& keyword) -> SyntaxDef* {
    return new SyntaxDef(keyword);
  }
};

class BeginDef : public Definition {
 private:
  DefinitionList definitions_;

  explicit BeginDef(const DefinitionList& definitions) :
    Definition(),
    definitions_(definitions) {}

 public:
  ~BeginDef() override = default;

  auto GetDefinitions() const -> const DefinitionList& {
    return definitions_;
  }

  auto GetTotalNumberOfDefinitions() const -> uint64_t {
    return definitions_.size();
  }

  auto GetDefinitionAt(const uint64_t idx) const -> Definition* {
    ASSERT(idx >= 0 && idx <= GetTotalNumberOfDefinitions());
    return definitions_[idx];
  }

  auto VisitChildren(NodeVisitor* vis) -> bool {
    for (const auto& defn : definitions_) {
      if (!defn->Accept(vis))
        return false;
    }
    return true;
  }

  DECLARE_NODE_TYPE(BeginDef);

 public:
  static inline auto New(const DefinitionList& definitions) -> BeginDef* {
    return new BeginDef(definitions);
  }
};

class Expression : public Form {
  DEFINE_NON_COPYABLE_TYPE(Expression);

 protected:
  Expression() = default;

 public:
  ~Expression() override = default;

  auto IsExpression() const -> bool override {
    return true;
  }
};

enum BinaryOp {
  kAddOp,
  kSubtractOp,
  kMultiplyOp,
  kDivideOp,
  kModOp,
};

static inline auto operator<<(std::ostream& stream, const BinaryOp& rhs) -> std::ostream& {
  switch (rhs) {
    case kAddOp:
      return stream << '+';
    case kSubtractOp:
      return stream << '-';
    case kDivideOp:
      return stream << '/';
    case kMultiplyOp:
      return stream << '*';
    case kModOp:
      return stream << '%';
    default:
      return stream << "Unknown BinaryOp";
  }
}

class BinaryOpExpr : public Expression {
 private:
  BinaryOp op_;
  Value* lhs_;
  Value* rhs_;

  BinaryOpExpr(const BinaryOp op, Value* lhs, Value* rhs) :
    Expression(),
    op_(op),
    lhs_(lhs),
    rhs_(rhs) {}

 public:
  ~BinaryOpExpr() override = default;

  auto GetOp() const -> BinaryOp {
    return op_;
  }

  auto GetLeft() const -> Value* {
    return lhs_;
  }

  auto HasLeft() const -> bool {
    return GetLeft() != nullptr;
  }

  auto GetRight() const -> Value* {
    return rhs_;
  }

  auto HasRight() const -> bool {
    return GetRight() != nullptr;
  }

  auto VisitChildren(NodeVisitor* vis) -> bool;

  DECLARE_NODE_TYPE(BinaryOpExpr);

 public:
  static inline auto New(const BinaryOp op, Value* lhs, Value* rhs) -> BinaryOpExpr* {
    return new BinaryOpExpr(op, lhs, rhs);
  }
};

class ExpressionList : public Expression {
  using StorageType = std::vector<Expression*>;

 private:
  StorageType expressions_;

  explicit ExpressionList(StorageType init = {}) :
    expressions_(std::move(init)) {}

 public:
  ~ExpressionList() override = default;

  auto GetAllExpressions() const -> const StorageType& {
    return expressions_;
  }

  auto GetTotalNumberOfExpressions() const -> uint64_t {
    return expressions_.size();
  }

  auto GetExpressionAt(const uint64_t idx) const -> Expression* {
    ASSERT(idx >= 0 && idx <= GetTotalNumberOfExpressions());
    return expressions_[idx];
  }

  auto VisitChildren(NodeVisitor* vis) -> bool {
    for (const auto& defn : expressions_) {
      if (!defn->Accept(vis))
        return false;
    }
    return true;
  }

  void Append(Expression* expr) {
    ASSERT(expr);
    expressions_.push_back(expr);
  }

  DECLARE_NODE_TYPE(ExpressionList);

 public:
  static inline auto New(const StorageType& init = {}) -> ExpressionList* {
    return new ExpressionList(init);
  }
};

class ConstantExpr : public Expression {
 private:
  Datum* value_;

  explicit ConstantExpr(Datum* value) :
    Expression(),
    value_(value) {
    ASSERT(value);
  }

 public:
  ~ConstantExpr() override = default;

  auto GetValue() const -> Datum* {
    return value_;
  }

  auto IsConstantExpr() const -> bool override {
    return true;
  }

  DECLARE_NODE_TYPE(ConstantExpr);

 public:
  static inline auto New(Datum* value) -> ConstantExpr* {
    ASSERT(value);
    return new ConstantExpr(value);
  }
};

class QuoteExpr : public Expression {
 private:
  QuoteExpr() = default;

 public:
  ~QuoteExpr() override = default;
  DECLARE_NODE_TYPE(QuoteExpr);

 public:
  static inline auto New() -> QuoteExpr* {
    return new QuoteExpr();
  }
};

class CallProcExpr : public Definition {
 private:
  Symbol* symbol_;
  Definition* args_;

  explicit CallProcExpr(Symbol* symbol, Definition* args) :
    Definition(),
    symbol_(symbol),
    args_(args) {}

 public:
  ~CallProcExpr() override = default;

  auto GetSymbol() const -> Symbol* {
    return symbol_;
  }

  auto GetArgs() const -> Definition* {
    return args_;
  }

  DECLARE_NODE_TYPE(CallProcExpr);

 public:
  static inline auto New(Symbol* symbol, Definition* args) -> CallProcExpr* {
    return new CallProcExpr(symbol, args);
  }

  static inline auto New(const std::string& symbol, Definition* args) -> CallProcExpr* {
    return New(Symbol::New(symbol), args);
  }
};

class LoadVariableExpr : public Expression {
 private:
  Variable* var_;

  explicit LoadVariableExpr(Variable* var) :
    Expression(),
    var_(var) {}

 public:
  ~LoadVariableExpr() override = default;

  auto GetVariable() const -> Variable* {
    return var_;
  }

  DECLARE_NODE_TYPE(LoadVariableExpr);

 public:
  static inline auto New(Variable* var) -> LoadVariableExpr* {
    ASSERT(var);
    return new LoadVariableExpr(var);
  }
};

class Body : public Node {
 private:
  DefinitionList definitions_{};
  std::vector<Expression*> expressions_{};

  Body() = default;

  void Append(Definition* defn) {
    ASSERT(defn);
    definitions_.push_back(defn);
  }

  void AppendAll(const DefinitionList& rhs) {
    std::for_each(std::begin(rhs), std::end(rhs), [this](Definition* defn) {
      Append(defn);
    });
  }

  void Append(Expression* expr) {
    ASSERT(expr);
    expressions_.push_back(expr);
  }

  void AppendAll(const std::vector<Expression*>& rhs) {
    std::for_each(std::begin(rhs), std::end(rhs), [this](Expression* expr) {
      Append(expr);
    });
  }

 public:
  ~Body() override = default;

  auto GetDefinitions() const -> const DefinitionList& {
    return definitions_;
  }

  auto GetTotalNumberOfDefinitions() const -> uint64_t {
    return definitions_.size();
  }

  auto GetExpressions() const -> const std::vector<Expression*>& {
    return expressions_;
  }

  auto GetTotalNumberOfExpressions() const -> uint64_t {
    return expressions_.size();
  }

  DECLARE_NODE_TYPE(Body);
};

class Value {
  DEFINE_NON_COPYABLE_TYPE(Value);

 private:
  Node* value_;

  explicit Value(Node* value) :
    value_(value) {
    ASSERT(value);
  }

 public:
  ~Value() = default;

  auto GetValue() const -> Node* {
    return value_;
  }

  auto IsExpression() const -> bool {
    return GetValue()->IsExpression();
  }

  auto IsBody() const -> bool {
    return GetValue()->IsBody();
  }

  auto Accept(NodeVisitor* vis) -> bool {
    ASSERT(vis);
    return GetValue()->Accept(vis);
  }

  auto ToString() const -> std::string;

 public:
  static inline auto New(Expression* expr) -> Value* {
    return new Value(expr);
  }

  static inline auto New(Body* body) -> Value* {
    return new Value(body);
  }
};

static inline auto operator<<(std::ostream& stream, Value* rhs) -> std::ostream& {
  ASSERT(rhs);
  return stream << rhs->ToString();
}
}  // namespace scm::ast

#endif  // SCM_AST_H
