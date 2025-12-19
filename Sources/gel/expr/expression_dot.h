#ifndef GEL_EXPRESSION_DOT_H
#define GEL_EXPRESSION_DOT_H

#ifdef GEL_ENABLE_GRAPHVIZ

#include <cstdint>
#include <filesystem>
#include <fmt/format.h>
#include <glog/logging.h>
#include <string>

#include "common.h"
#include "expr/expression.h"
#include "gv.h"
#include "symbol.h"

namespace gel {
namespace expr {
class ExpressionToDot : public dot::GraphBuilder, public ExpressionVisitor {
  friend class NodeScope;
  DEFINE_NON_COPYABLE_TYPE(ExpressionToDot);

 private:
  class NodeScope {
    DEFINE_NON_COPYABLE_TYPE(NodeScope);

   private:
    ExpressionToDot* owner_ = nullptr;
    dot::Node* previous_ = nullptr;
    dot::Node* current_ = nullptr;

    inline void SetOwner(ExpressionToDot* owner) {
      ASSERT(owner);
      owner_ = owner;
    }

    inline void SetPrevious(dot::Node* node) {
      ASSERT(node);
      previous_ = node;
    }

    inline void SetCurrent(dot::Node* node) {
      ASSERT(node);
      current_ = node;
    }

   public:
    explicit NodeScope(ExpressionToDot* owner, dot::Node* current) {
      SetOwner(owner);
      if (!HasOwner())
        return;
      SetCurrent(current);
      if (GetOwner()->HasParent())
        SetPrevious(GetOwner()->GetParent());
      GetOwner()->SetParent(current);
    }
    ~NodeScope() {
      if (HasOwner() && HasPrevious())
        GetOwner()->SetParent(GetPrevious());
    }

    auto GetOwner() const -> ExpressionToDot* {
      return owner_;
    }

    auto HasOwner() const -> bool {
      return GetOwner() != nullptr;
    }

    auto GetPrevious() const -> dot::Node* {
      return previous_;
    }

    auto HasPrevious() const -> bool {
      return GetPrevious() != nullptr;
    }
  };

  inline auto NextNodeId() -> std::string {
    return fmt::format("e{0:d}", num_expressions_++);
  }

  inline auto NewNode() -> dot::Node* {
    const auto node_id = NextNodeId();
    return dot::NewNode(GetGraph(), node_id.c_str());
  }

 private:
  dot::Node* parent_ = nullptr;
  dot::EdgeList edges_{};
  uint64_t num_expressions_ = 0;

  inline void SetParent(dot::Node* node) {
    ASSERT(node);
    parent_ = node;
  }

  inline auto GetParent() const -> dot::Node* {
    return parent_;
  }

  inline auto HasParent() const -> bool {
    return GetParent() != nullptr;
  }

  inline void CreateEdgeFromParent(dot::Node* node) {
    if (!HasParent())
      return;
    ASSERT(node);
    const auto edge = dot::NewEdge(GetGraph(), fmt::format("e{0:d}", edges_.size()).c_str(), GetParent(), node);
    ASSERT(edge);
    edges_.push_back(edge);
  }

  auto ProcessChildren(Expression* expr, dot::Node* node) -> bool;

 public:
  explicit ExpressionToDot(const char* graph_name);
  ~ExpressionToDot() override = default;

  auto Build() -> dot::DotGraph* override;
#define DEFINE_VISIT(Name) auto Visit##Name(Name* expr)->bool override;
  FOR_EACH_EXPRESSION_NODE(DEFINE_VISIT)
#undef DEFINE_VISIT
 public:
  static inline auto BuildGraph(const char* name, Expression* expr) -> dot::DotGraph* {
    ASSERT(name);
    ASSERT(expr);
    ExpressionToDot builder(name);
    if (!expr->Accept(builder)) {
      DLOG(ERROR) << "failed to visit: " << expr->ToString();
      return nullptr;
    }
    return builder.Build();
  }

  static inline auto BuildGraph(const std::string& name, Expression* expr) -> dot::DotGraph* {
    ASSERT(!name.empty());
    ASSERT(expr);
    return BuildGraph(name.c_str(), expr);
  }

  static inline auto BuildGraph(Symbol* symbol, Expression* expr) -> dot::DotGraph* {
    ASSERT(symbol);
    ASSERT(expr);
    return BuildGraph(symbol->GetSymbolName(), expr);
  }
};

void GenerateExprDotPng(const std::filesystem::path& file, const std::string& name, Expression* expr);
}  // namespace expr
using namespace expr;
}  // namespace gel

#endif  // GEL_ENABLE_GRAPHVIZ
#endif  // GEL_EXPRESSION_DOT_H
