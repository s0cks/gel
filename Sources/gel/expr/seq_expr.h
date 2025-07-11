#ifndef GEL_SEQ_EXPR_H
#define GEL_SEQ_EXPR_H

#include "gel/expr/expression.h"

namespace gel::expr {
class SeqExpr : public Expression {
  friend class gel::Parser;

 private:
  ExpressionList children_{};

  inline auto at(const uint64_t idx) const -> expr::ExpressionList::const_iterator {
    return std::begin(children_) + static_cast<expr::ExpressionList::difference_type>(idx);
  }

 protected:
  SeqExpr(const ExpressionList& children) {
    children_.insert(std::end(children_), std::begin(children), std::end(children));
  }

 public:
  ~SeqExpr() override = default;

  auto GetBody() const -> const ExpressionList& {
    return children_;
  }

  auto GetNumberOfChildren() const -> uint64_t override {
    return children_.size();
  }

  virtual auto IsEmpty() const -> bool {
    return GetNumberOfChildren() == 0;
  }

  void InsertAt(const uint64_t idx, Expression* child) {
    ASSERT(idx >= 0 && idx <= GetNumberOfChildren());
    ASSERT(child);
    children_.insert(at(idx), child);
  }

  void InsertAt(const uint64_t idx, const ExpressionList& children) {
    ASSERT(idx >= 0 && idx <= GetNumberOfChildren());
    ASSERT(!children.empty());
    children_.insert(at(idx), std::begin(children), std::end(children));
  }

  auto GetChildAt(const uint64_t idx) const -> Expression* override {
    ASSERT(idx >= 0 && idx <= GetNumberOfChildren());
    return children_[idx];
  }

  void SetChildAt(const uint64_t idx, Expression* value) override {
    ASSERT(idx >= 0 && idx <= GetNumberOfChildren());
    ASSERT(value);
    children_[idx] = value;
  }

  void Append(Expression* expr) {
    ASSERT(expr);
    children_.push_back(expr);
  }

  void ReplaceChildAt(const uint64_t idx, expr::Expression* child) {
    ASSERT(idx >= 0 && idx <= GetNumberOfChildren());
    ASSERT(child);
    RemoveChildAt(idx);
    InsertAt(idx, child);
  }

  void ReplaceChildAt(const uint64_t idx, const ExpressionList& children) {
    ASSERT(idx >= 0 && idx <= GetNumberOfChildren());
    ASSERT(!children.empty());
    RemoveChildAt(idx);
    InsertAt(idx, children);
  }

  void RemoveChildAt(const uint64_t idx) override {
    ASSERT(idx >= 0 && idx <= GetNumberOfChildren());
    children_.erase(children_.begin() + static_cast<word>(idx));
  }

  auto GetLastExpr() const -> Expression* {
    return IsEmpty() ? nullptr : children_.back();
  }

  auto IsConstantExpr() const -> bool override;
  auto VisitChildren(ExpressionVisitor& vis) -> bool override;
  auto VisitAllDefinitions(ExpressionVisitor& vis) -> bool override;
  DECLARE_EXPRESSION(SeqExpr);

 public:
  static inline auto New(const expr::ExpressionList& children = {}) -> SeqExpr* {
    return new SeqExpr(children);
  }

  static inline auto New(expr::Expression* expr) -> SeqExpr* {
    ASSERT(expr);
    return New(expr::ExpressionList{expr});
  }
};
}  // namespace gel::expr

#endif  // GEL_SEQ_EXPR_H
