#ifndef SCM_EXPRESSION_H
#define SCM_EXPRESSION_H

#include <ostream>
#include <string>
#include <utility>

#include "scheme/common.h"
#include "scheme/lambda.h"
#include "scheme/type.h"
#include "scheme/variable.h"

namespace scm {
class Parser;

namespace expr {
#define FOR_EACH_EXPRESSION_NODE(V) \
  V(BinaryOpExpr)                   \
  V(LiteralExpr)                    \
  V(BeginExpr)                      \
  V(EvalExpr)                       \
  V(SymbolExpr)                     \
  V(CallProcExpr)                   \
  V(CondExpr)                       \
  V(LambdaExpr)                     \
  V(LocalDef)                       \
  V(ModuleDef)

class Expression;
class Definition;
#define FORWARD_DECLARE(Name) class Name;
FOR_EACH_EXPRESSION_NODE(FORWARD_DECLARE)
#undef FORWARD_DECLARE

class ExpressionVisitor {
  DEFINE_NON_COPYABLE_TYPE(ExpressionVisitor);
#define FORWARD_DECLARE(Name) friend class Name;
  FOR_EACH_EXPRESSION_NODE(FORWARD_DECLARE)
#undef FORWARD_DECLARE

 protected:
  ExpressionVisitor() = default;

#define DEFINE_VISIT(Name) virtual auto Visit##Name(Name* expr) -> bool = 0;
  FOR_EACH_EXPRESSION_NODE(DEFINE_VISIT)
#undef DEFINE_VISIT
 public:
  virtual ~ExpressionVisitor() = default;
};

class Expression {
  DEFINE_NON_COPYABLE_TYPE(Expression);

 protected:
  Expression() = default;

  virtual void SetChildAt(const uint64_t idx, Expression* expr) {
    // do nothing
  }

 public:
  virtual ~Expression() = default;
  virtual auto GetName() const -> const char* = 0;
  virtual auto ToString() const -> std::string = 0;
  virtual auto Accept(ExpressionVisitor* vis) -> bool = 0;

  virtual auto GetNumberOfChildren() const -> uint64_t {
    return 0;
  }

  virtual auto GetChildAt(const uint64_t idx) const -> Expression* {
    return nullptr;
  }

  inline auto HasChildAt(const uint64_t idx) const -> bool {
    return GetChildAt(idx) != nullptr;
  }

  virtual auto IsConstantExpr() const -> bool {
    return false;
  }

  virtual auto EvalToConstant() const -> Type* {
    return nullptr;
  }

  virtual auto AsDefinition() -> Definition* {
    return nullptr;
  }

  inline auto IsDefinition() -> bool {
    return AsDefinition() != nullptr;
  }

  virtual auto VisitChildren(ExpressionVisitor* vis) -> bool {
    return false;
  }

#define DEFINE_TYPE_CHECK(Name)      \
  virtual auto As##Name() -> Name* { \
    return nullptr;                  \
  }                                  \
  auto Is##Name() -> bool {          \
    return As##Name() != nullptr;    \
  }
  FOR_EACH_EXPRESSION_NODE(DEFINE_TYPE_CHECK)
#undef DEFINE_TYPE_CHECK
};

template <const int NumInputs>
class TemplateExpression : public Expression {
 private:
  std::array<Expression*, NumInputs> children_{};

 protected:
  TemplateExpression() = default;

  void SetChildAt(const uint64_t idx, Expression* value) override {
    ASSERT(idx >= 0 && idx <= NumInputs);
    ASSERT(value);
    children_.at(idx) = value;
  }

 public:
  auto GetNumberOfChildren() const -> uint64_t override {
    return NumInputs;
  }

  auto GetChildAt(const uint64_t idx) const -> Expression* override {
    ASSERT(idx >= 0 && idx <= NumInputs);
    return children_.at(idx);
  }

  inline auto HasChildAt(const uint64_t idx) const -> bool {
    return GetChildAt(idx) != nullptr;
  }
};

using ExpressionList = std::vector<Expression*>;

#define DECLARE_EXPRESSION(Name)                        \
  friend class ExpressionVisitor;                       \
  DEFINE_NON_COPYABLE_TYPE(Name);                       \
                                                        \
 public:                                                \
  auto Accept(ExpressionVisitor* vis) -> bool override; \
  auto ToString() const -> std::string override;        \
  auto GetName() const -> const char* override {        \
    return #Name;                                       \
  }                                                     \
  auto As##Name() -> Name* override {                   \
    return this;                                        \
  }

class LiteralExpr : public Expression {
 private:
  Datum* value_;

 protected:
  explicit LiteralExpr(Datum* value) :
    Expression(),
    value_(value) {}

 public:
  ~LiteralExpr() override = default;

  auto GetValue() const -> Datum* {
    return value_;
  }

  auto IsConstantExpr() const -> bool override {
    return true;
  }

  auto EvalToConstant() const -> Type* override {
    return value_;
  }

  DECLARE_EXPRESSION(LiteralExpr);

 public:
  static inline auto New(Datum* value) -> LiteralExpr* {
    ASSERT(value);
    return new LiteralExpr(value);
  }
};

enum BinaryOp : uint64_t {
  kAdd,
  kSubtract,
  kMultiply,
  kDivide,
  kModulus,
  kBinaryAnd,
  kBinaryOr,
  kEquals,
};

static inline auto operator<<(std::ostream& stream, const BinaryOp& rhs) -> std::ostream& {
  switch (rhs) {
    case kAdd:
      return stream << "+";
    case kSubtract:
      return stream << "-";
    case kMultiply:
      return stream << "*";
    case kDivide:
      return stream << "/";
    case kModulus:
      return stream << "%";
    case kBinaryAnd:
      return stream << "&";
    case kBinaryOr:
      return stream << "|";
    default:
      return stream << "Unknown BinaryOp: " << static_cast<int64_t>(rhs);
  }
}

class BinaryOpExpr : public TemplateExpression<2> {
  static constexpr const auto kLeftInput = 0;
  static constexpr const auto kRightInput = 1;

 private:
  BinaryOp op_;

 protected:
  explicit BinaryOpExpr(const BinaryOp op, Expression* left, Expression* right) :
    TemplateExpression<2>(),
    op_(op) {
    SetLeft(left);
    SetRight(right);
  }

  inline void SetLeft(Expression* value) {
    ASSERT(value);
    SetChildAt(kLeftInput, value);
  }

  inline void SetRight(Expression* value) {
    ASSERT(value);
    SetChildAt(kRightInput, value);
  }

 public:
  ~BinaryOpExpr() override = default;

  auto GetOp() const -> BinaryOp {
    return op_;
  }

  auto GetLeft() const -> Expression* {
    return GetChildAt(kLeftInput);
  }

  inline auto HasLeft() const -> bool {
    return GetLeft() != nullptr;
  }

  auto GetRight() const -> Expression* {
    return GetChildAt(kRightInput);
  }

  inline auto HasRight() const -> bool {
    return GetRight() != nullptr;
  }

  auto IsConstantExpr() const -> bool override;
  auto EvalToConstant() const -> Type* override;
  auto VisitChildren(ExpressionVisitor* vis) -> bool override;
  DECLARE_EXPRESSION(BinaryOpExpr);

 public:
  static inline auto New(const BinaryOp op, Expression* left, Expression* right) -> BinaryOpExpr* {
    ASSERT(left);
    ASSERT(right);
    return new BinaryOpExpr(op, left, right);
  }
};

class SequenceExpr : public Expression {
  friend class scm::Parser;
  DEFINE_NON_COPYABLE_TYPE(SequenceExpr);

 private:
  ExpressionList children_{};

 protected:
  SequenceExpr(const ExpressionList& children) {
    children_.insert(std::end(children_), std::begin(children), std::end(children));
  }

  inline void Append(Expression* expr) {
    ASSERT(expr);
    children_.push_back(expr);
  }

  void SetChildAt(const uint64_t idx, Expression* value) override {
    ASSERT(idx >= 0 && idx <= GetNumberOfChildren());
    ASSERT(value);
    children_[idx] = value;
  }

 public:
  ~SequenceExpr() override = default;

  auto GetNumberOfChildren() const -> uint64_t override {
    return children_.size();
  }

  auto GetChildAt(const uint64_t idx) const -> Expression* override {
    ASSERT(idx >= 0 && idx <= GetNumberOfChildren());
    return children_[idx];
  }

  auto IsConstantExpr() const -> bool override;
  auto VisitChildren(ExpressionVisitor* vis) -> bool override;
};

class BeginExpr : public SequenceExpr {
 protected:
  explicit BeginExpr(const ExpressionList& expressions) :
    SequenceExpr(expressions) {}

 public:
  ~BeginExpr() = default;

  DECLARE_EXPRESSION(BeginExpr);

 public:
  static inline auto New(const ExpressionList& expressions = {}) -> BeginExpr* {
    return new BeginExpr(expressions);
  }
};

class EvalExpr : public TemplateExpression<1> {
 protected:
  explicit EvalExpr(Expression* expr) :
    TemplateExpression<1>() {
    SetChildAt(0, expr);
  }

 public:
  ~EvalExpr() = default;

  auto GetExpression() const -> Expression* {
    return GetChildAt(0);
  }

  auto HasExpression() const -> bool {
    return HasChildAt(0);
  }

  DECLARE_EXPRESSION(EvalExpr);

 public:
  static inline auto New(Expression* expr) -> EvalExpr* {
    ASSERT(expr);
    return new EvalExpr(expr);
  }
};

class CallProcExpr : public SequenceExpr {
 private:
  Symbol* symbol_ = nullptr;

 protected:
  explicit CallProcExpr(Symbol* symbol, const ExpressionList& args) :
    SequenceExpr(args) {
    SetSymbol(symbol);
  }

  inline void SetSymbol(Symbol* symbol) {
    ASSERT(symbol);
    symbol_ = symbol;
  }

 public:
  ~CallProcExpr() override = default;

  auto GetSymbol() const -> Symbol* {
    return symbol_;
  }

  inline auto HasSymbol() const -> bool {
    return GetSymbol() != nullptr;
  }

  DECLARE_EXPRESSION(CallProcExpr);

 public:
  static inline auto New(Symbol* symbol, const ExpressionList& args = {}) -> CallProcExpr* {
    return new CallProcExpr(symbol, args);
  }
};

class SymbolExpr : public TemplateExpression<0> {
 private:
  Symbol* symbol_ = nullptr;

 protected:
  explicit SymbolExpr(Symbol* symbol) :
    TemplateExpression<0>() {
    SetSymbol(symbol);
  }

  inline void SetSymbol(Symbol* symbol) {
    ASSERT(symbol);
    symbol_ = symbol;
  }

 public:
  ~SymbolExpr() override = default;

  auto GetSymbol() const -> Symbol* {
    return symbol_;
  }

  inline auto HasSymbol() const -> bool {
    return GetSymbol() != nullptr;
  }

  DECLARE_EXPRESSION(SymbolExpr);

 public:
  static inline auto New(Symbol* symbol) -> SymbolExpr* {
    ASSERT(symbol);
    return new SymbolExpr(symbol);
  }
};

class CondExpr : public Expression {
 private:
  Expression* test_ = nullptr;
  Expression* conseq_ = nullptr;
  Expression* alt_ = nullptr;

 protected:
  CondExpr(Expression* test, Expression* conseq, Expression* alt) :
    Expression() {
    SetTest(test);
    SetConseq(conseq);
    if (alt)
      SetAlt(alt);
  }

  inline void SetTest(Expression* expr) {
    ASSERT(expr);
    test_ = expr;
  }

  inline void SetConseq(Expression* expr) {
    ASSERT(expr);
    conseq_ = expr;
  }

  inline void SetAlt(Expression* expr) {
    ASSERT(expr);
    alt_ = expr;
  }

 public:
  ~CondExpr() override = default;

  auto GetTest() const -> Expression* {
    return test_;
  }

  auto GetConseq() const -> Expression* {
    return conseq_;
  }

  auto GetAlternate() const -> Expression* {
    return alt_;
  }

  inline auto HasAlternate() const -> bool {
    return GetAlternate() != nullptr;
  }

  auto VisitChildren(ExpressionVisitor* vis) -> bool override;
  DECLARE_EXPRESSION(CondExpr);

 public:
  static inline auto New(Expression* test, Expression* conseq, Expression* alt = nullptr) -> CondExpr* {
    ASSERT(test);
    ASSERT(conseq);
    return new CondExpr(test, conseq, alt);
  }
};

class LambdaExpr : public Expression {
  friend class scm::Parser;

 protected:
  ArgumentSet args_;
  Expression* body_;

  explicit LambdaExpr(const ArgumentSet& args, Expression* body) :
    Expression(),
    args_(args),
    body_(body) {}

 public:
  ~LambdaExpr() override = default;

  auto GetArgs() const -> const ArgumentSet& {
    return args_;
  }

  auto GetBody() const -> Expression* {
    return body_;
  }

  DECLARE_EXPRESSION(LambdaExpr);

 public:
  static inline auto New(const ArgumentSet& args, Expression* body) -> LambdaExpr* {
    return new LambdaExpr(args, body);
  }
};

// Definitions

class Definition : public Expression {
  DEFINE_NON_COPYABLE_TYPE(Definition);

 protected:
  Definition() = default;

 public:
  ~Definition() override = default;

  auto AsDefinition() -> Definition* override {
    return this;
  }
};

using DefinitionList = std::vector<Definition*>;

static inline auto operator<<(std::ostream& stream, const DefinitionList& rhs) -> std::ostream& {
  stream << "[";
  auto remaining = rhs.size();
  for (const auto& defn : rhs) {
    stream << defn->ToString();
    if (--remaining >= 1)
      stream << ",";
  }
  stream << "]";
  return stream;
}

template <const uint64_t NumInputs>
class TemplateDefinition : public Definition {
  DEFINE_NON_COPYABLE_TYPE(TemplateDefinition<NumInputs>);

 private:
  std::array<Expression*, NumInputs> children_{};

 protected:
  TemplateDefinition() = default;

  void SetChildAt(const uint64_t idx, Expression* value) override {
    ASSERT(idx >= 0 && idx <= NumInputs);
    ASSERT(value);
    children_[idx] = value;  // NOLINT
  }

 public:
  ~TemplateDefinition() override = default;

  auto GetNumberOfChildren() const -> uint64_t override {
    return NumInputs;
  }

  auto GetChildAt(const uint64_t idx) const -> Expression* override {
    ASSERT(idx >= 0 && idx <= NumInputs);
    return children_[idx];  // NOLINT
  }

  auto VisitChildren(ExpressionVisitor* vis) -> bool override {
    ASSERT(vis);
    for (const auto& child : children_) {
      if (!child->Accept(vis))
        return false;
    }
    return true;
  }
};

class LocalDef : public TemplateDefinition<1> {
 private:
  Symbol* symbol_ = nullptr;

  inline void SetSymbol(Symbol* symbol) {
    ASSERT(symbol);
    symbol_ = symbol;
  }

 protected:
  LocalDef(Symbol* symbol, Expression* value) :
    TemplateDefinition<1>() {
    SetSymbol(symbol);
    SetChildAt(0, value);
  }

 public:
  ~LocalDef() override = default;

  auto GetSymbol() const -> Symbol* {
    return symbol_;
  }

  auto GetValue() const -> Expression* {
    return GetChildAt(0);
  }

  auto HasValue() const -> bool {
    return GetValue() != nullptr;
  }

  auto VisitChildren(ExpressionVisitor* vis) -> bool override {
    ASSERT(vis);
    return GetValue()->Accept(vis);
  }

  DECLARE_EXPRESSION(LocalDef);

 public:
  static inline auto New(Symbol* symbol, Expression* value) -> LocalDef* {
    ASSERT(symbol);
    ASSERT(value);
    return new LocalDef(symbol, value);
  }
};

class ModuleDef : public Definition {
  friend class scm::Parser;

 private:
  Symbol* symbol_ = nullptr;
  DefinitionList definitions_{};

 protected:
  ModuleDef(Symbol* symbol) :
    Definition() {
    SetSymbol(symbol);
  }

  inline void SetSymbol(Symbol* symbol) {
    ASSERT(symbol);
    symbol_ = symbol;
  }

  void SetChildAt(const uint64_t idx, Expression* expr) override {
    ASSERT(idx >= 0 && idx < GetNumberOfChildren())
    ASSERT(expr);
    ASSERT(expr->IsDefinition());
    definitions_[idx] = expr->AsDefinition();
  }

  inline void Append(Definition* defn) {
    ASSERT(defn);
    definitions_.push_back(defn);
  }

 public:
  ~ModuleDef() override = default;

  auto GetSymbol() const -> Symbol* {
    return symbol_;
  }

  auto GetNumberOfChildren() const -> uint64_t override {
    return definitions_.size();
  }

  auto GetChildAt(const uint64_t idx) const -> Expression* override {
    ASSERT(idx >= 0 && idx <= GetNumberOfChildren());
    return definitions_[idx];
  }

  auto GetDefinitionAt(const uint64_t idx) const -> Definition* {
    return GetChildAt(idx)->AsDefinition();
  }

  auto VisitChildren(ExpressionVisitor* vis) -> bool override;
  DECLARE_EXPRESSION(ModuleDef);

 public:
  static inline auto New(Symbol* symbol) -> ModuleDef* {
    ASSERT(symbol);
    return new ModuleDef(symbol);
  }
};
}  // namespace expr

using expr::BinaryOp;
using expr::Expression;
using expr::ExpressionList;
using expr::ExpressionVisitor;
#define DEFINE_USE(Name) using expr::Name;
FOR_EACH_EXPRESSION_NODE(DEFINE_USE)
#undef DEFINE_USE
}  // namespace scm

#undef DECLARE_EXPRESSION
#endif  // SCM_EXPRESSION_H
