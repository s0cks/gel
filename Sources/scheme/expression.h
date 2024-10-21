#ifndef SCM_EXPRESSION_H
#define SCM_EXPRESSION_H

#include <ostream>
#include <string>
#include <utility>

#include "scheme/common.h"
#include "scheme/type.h"
#include "scheme/variable.h"

namespace scm {
class Parser;

namespace expr {
#define FOR_EACH_EXPRESSION_NODE(V) \
  V(BinaryOp)                       \
  V(Literal)                        \
  V(Begin)                          \
  V(Eval)                           \
  V(Define)                         \
  V(Symbol)                         \
  V(CallProc)

class Expression;
#define FORWARD_DECLARE(Name) class Name##Expr;
FOR_EACH_EXPRESSION_NODE(FORWARD_DECLARE)
#undef FORWARD_DECLARE

class ExpressionVisitor {
  DEFINE_NON_COPYABLE_TYPE(ExpressionVisitor);
#define FORWARD_DECLARE(Name) friend class Name##Expr;
  FOR_EACH_EXPRESSION_NODE(FORWARD_DECLARE)
#undef FORWARD_DECLARE

 protected:
  ExpressionVisitor() = default;

#define DEFINE_VISIT(Name) virtual auto Visit##Name(Name##Expr* expr) -> bool = 0;
  FOR_EACH_EXPRESSION_NODE(DEFINE_VISIT)
#undef DEFINE_VISIT
 public:
  virtual ~ExpressionVisitor() = default;
};

class Expression {
  DEFINE_NON_COPYABLE_TYPE(Expression);

 protected:
  Expression() = default;

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

  virtual auto IsConstantExpr() const -> bool {
    return false;
  }

  virtual auto VisitChildren(ExpressionVisitor* vis) -> bool {
    return false;
  }

#define DEFINE_TYPE_CHECK(Name)            \
  virtual auto As##Name() -> Name##Expr* { \
    return nullptr;                        \
  }                                        \
  auto Is##Name() -> bool {                \
    return As##Name() != nullptr;          \
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

  void SetChildAt(const uint64_t idx, Expression* value) {
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
  DEFINE_NON_COPYABLE_TYPE(Name##Expr)                  \
 public:                                                \
  auto Accept(ExpressionVisitor* vis) -> bool override; \
  auto ToString() const -> std::string override;        \
  auto GetName() const -> const char* override {        \
    return #Name;                                       \
  }                                                     \
  auto As##Name() -> Name##Expr* override {             \
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

  DECLARE_EXPRESSION(Literal);

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

  auto GetRight() const -> Expression* {
    return GetChildAt(kRightInput);
  }

  auto IsConstantExpr() const -> bool override;
  auto VisitChildren(ExpressionVisitor* vis) -> bool override;
  DECLARE_EXPRESSION(BinaryOp);

 public:
  static inline auto New(const BinaryOp op, Expression* left, Expression* right) -> BinaryOpExpr* {
    ASSERT(left);
    ASSERT(right);
    return new BinaryOpExpr(op, left, right);
  }
};

class DefineExpr : public TemplateExpression<1> {
 private:
  Symbol* symbol_ = nullptr;

  inline void SetSymbol(Symbol* symbol) {
    ASSERT(var);
    symbol_ = symbol;
  }

 protected:
  DefineExpr(Symbol* symbol, Expression* value) :
    TemplateExpression<1>() {
    SetSymbol(symbol);
    SetChildAt(0, value);
  }

 public:
  ~DefineExpr() override = default;

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

  DECLARE_EXPRESSION(Define);

 public:
  static inline auto New(Symbol* symbol, Expression* value) -> DefineExpr* {
    ASSERT(symbol);
    ASSERT(value);
    return new DefineExpr(symbol, value);
  }
};

class SequenceExpr : public Expression {
  friend class scm::Parser;
  DEFINE_NON_COPYABLE_TYPE(SequenceExpr);

 private:
  ExpressionList children_;

 protected:
  SequenceExpr(ExpressionList children) :
    children_(std::move(children)) {}

  inline void Append(Expression* expr) {
    ASSERT(expr);
    children_.push_back(expr);
  }

  inline void SetChildAt(const uint64_t idx, Expression* value) {
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
    SequenceExpr(expressions) {
    ASSERT(!expressions.empty());
  }

 public:
  ~BeginExpr() = default;

  DECLARE_EXPRESSION(Begin);

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

  DECLARE_EXPRESSION(Eval);

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

  DECLARE_EXPRESSION(CallProc);

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

  DECLARE_EXPRESSION(Symbol);

 public:
  static inline auto New(Symbol* symbol) -> SymbolExpr* {
    ASSERT(symbol);
    return new SymbolExpr(symbol);
  }
};
}  // namespace expr

using namespace expr;  // TODO: remove
}  // namespace scm

#undef DECLARE_EXPRESSION
#endif  // SCM_EXPRESSION_H
