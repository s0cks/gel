#ifndef SCM_GV_H
#define SCM_GV_H

#include <graphviz/cgraph.h>
#include <graphviz/gvcext.h>

#include <string>
#include <utility>
#include <vector>

#include "scheme/common.h"

namespace scm {
class DotGraph {
  using Symbol = Agsym_t;
  DEFINE_NON_COPYABLE_TYPE(DotGraph);

 private:
  static Symbol* kNodeLabelAttr;

 private:
  Agraph_t* graph_;

  explicit DotGraph(Agraph_t* graph);

 public:
  ~DotGraph();

  auto get() const -> Agraph_t* {
    return graph_;
  }

  void RenderTo(FILE* stream);
  void RenderPngTo(FILE* stream);
  void RenderPngToFilename(const std::string& filename);

  inline void RenderToStdout() {
    return RenderTo(stdout);
  }

 public:
  static void Init();
  static auto New(Agraph_t* graph) -> DotGraph*;
};

class DotGraphBuilder {
  DEFINE_NON_COPYABLE_TYPE(DotGraphBuilder);

 public:
  static constexpr const auto kDefaultEdgeFlags = 1;
  static constexpr const auto kDefaultNodeFlags = 1;
  using Graph = Agraph_t;
  using Node = Agnode_t;
  using NodeList = std::vector<Node*>;
  using Edge = Agedge_t;
  using EdgeList = std::vector<Edge*>;

 private:
  Agraph_t* graph_;  // TODO: memory leak
 protected:
  DotGraphBuilder(const char* name, Agdesc_t desc);

  inline void SetGraph(Agraph_t* g) {
    ASSERT(g);
    graph_ = g;
  }

  inline auto GetGraph() const -> Graph* {
    return graph_;
  }

  inline auto HasGraph() const -> bool {
    return GetGraph() != nullptr;
  }

  inline void SetAttr(int kind, const std::string& name, const std::string& value) {
    agattr(GetGraph(), kind, const_cast<char*>(name.c_str()), const_cast<char*>(value.c_str()));  // NOLINT
  }

  inline void SetNodeAttr(const std::string& name, const std::string& value) {
    return SetAttr(AGNODE, name, value);
  }

  inline void SetGraphAttr(const std::string& name, const std::string& value) {
    return SetAttr(AGRAPH, name, value);
  }

  inline void SetEdgeAttr(const std::string& name, const std::string& value) {
    return SetAttr(AGEDGE, name, value);
  }

  inline auto NewNode(const char* name, const int flags = kDefaultNodeFlags) -> Node* {
    ASSERT(HasGraph());
    ASSERT(name);
    return agnode(GetGraph(), const_cast<char*>(name), flags);
  }

  inline auto NewNode(const std::string& name, const int flags = kDefaultNodeFlags) -> Node* {
    ASSERT(HasGraph());
    ASSERT(!name.empty());
    return NewNode(name.c_str(), flags);
  }

  inline auto NewEdge(Node* from, Node* to, const std::string& name = "", const int flags = kDefaultEdgeFlags) -> Edge* {
    ASSERT(from);
    ASSERT(to);
    return agedge(GetGraph(), from, to, const_cast<char*>(name.c_str()), flags);  // NOLINT
  }

  inline void SetNodeLabel(Node* node, const std::string& value) {
    static constexpr const auto kLabelKey = "label";
    ASSERT(node);
    ASSERT(!value.empty());
    agset(node, const_cast<char*>(kLabelKey), value.c_str());  // NOLINT
  }

 public:
  virtual ~DotGraphBuilder() = default;
  virtual auto BuildDotGraph() -> DotGraph* = 0;
};

class DotGraphRenderer {
  DEFINE_NON_COPYABLE_TYPE(DotGraphRenderer);

 private:
  GVC_t* context_;

  void SetContext(GVC_t* ctx) {
    ASSERT(ctx);
    context_ = ctx;
  }

  auto GetContext() const -> GVC_t* {
    return context_;
  }

  auto HasContext() const -> bool {
    return GetContext() != nullptr;
  }

 public:
  DotGraphRenderer();
  ~DotGraphRenderer();
  void RenderTo(DotGraph* graph, FILE* stream, const std::string& layout, const std::string& format);
  void RenderDotTo(DotGraph* graph, FILE* stream);

  inline void RenderDotToStdout(DotGraph* graph) {
    return RenderDotTo(graph, stdout);
  }

  inline void RenderPngTo(DotGraph* graph, FILE* stream, const std::string& layout = "dot") {
    ASSERT(stream);
    ASSERT(graph);
    ASSERT(!layout.empty());
    return RenderTo(graph, stream, layout, "png");
  }
};
}  // namespace scm

#endif  // SCM_GV_H
