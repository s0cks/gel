#ifndef SCM_GV_H
#define SCM_GV_H

#include <fmt/format.h>
#include <glog/logging.h>
#include <graphviz/cgraph.h>
#include <graphviz/gvc.h>
#include <graphviz/gvcext.h>

#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "scheme/common.h"

namespace scm::dot {
using Symbol = Agsym_t;
class GraphBuilder;
class Graph {
  using Handle = Agraph_t;
  DEFINE_NON_COPYABLE_TYPE(Graph);

 private:
  Handle* handle_ = nullptr;

  explicit Graph(Handle* handle) {
    SetHandle(handle);
  }

  inline void SetHandle(Handle* handle) {
    ASSERT(handle);
    handle_ = handle;
  }

 public:
  ~Graph();

  auto get() const -> Handle* {
    return handle_;
  }

  void RenderTo(FILE* stream);
  void RenderPngTo(FILE* stream);
  void RenderPngToFilename(const std::string& filename);

  inline void RenderToStdout() {
    return RenderTo(stdout);
  }

 public:
  static inline auto New(Handle* handle) -> Graph* {
    return new Graph(handle);
  }

  static inline auto New(const char* name, Agdesc_t desc, Agdisc_t* disc = nullptr) -> Graph* {
    return New(agopen(const_cast<char*>(name), desc, disc));  // NOLINT
  }

  static inline auto New(const std::string& name, Agdesc_t desc, Agdisc_t* disc = nullptr) -> Graph* {
    return New(name.c_str(), desc, disc);
  }

  static auto New(GraphBuilder* builder) -> Graph*;
};

class GraphDecorator {
  DEFINE_NON_COPYABLE_TYPE(GraphDecorator);

 protected:
  using Graph = Agraph_t;
  using Node = Agnode_t;
  using NodeList = std::vector<Node*>;
  using Edge = Agedge_t;
  using EdgeList = std::vector<Edge*>;

 private:
  Graph* graph_ = nullptr;

  inline void SetGraph(Graph* graph) {
    ASSERT(graph);
    graph_ = graph;
  }

  template <typename T>
  inline auto Set(T* obj, const char* name, const char* value) -> int {
    ASSERT(obj);
    ASSERT(name);
    ASSERT(value);
    return agset(obj, const_cast<char*>(name), value);  // NOLINT(cppcoreguidelines-pro-type-const-cast)
  }

  template <const bool Create>
  inline auto N(const char* name) -> Node* {
    ASSERT(name);
    return agnode(GetGraph(), const_cast<char*>(name), Create);  // NOLINT(cppcoreguidelines-pro-type-const-cast)
  }

  template <const bool Create>
  inline auto E(Node* from, Node* to, const char* name) -> Edge* {
    ASSERT(from);
    ASSERT(to);
    ASSERT(name);
    return agedge(GetGraph(), from, to, const_cast<char*>(name), Create);  // NOLINT(cppcoreguidelines-pro-type-const-cast)
  }

 protected:
  explicit GraphDecorator(Graph* graph) {
    SetGraph(graph);
  }

  inline void SetAttr(int kind, const char* name, const char* value) {
    ASSERT(name);
    ASSERT(value);
    agattr(GetGraph(), kind, const_cast<char*>(name), const_cast<char*>(value));  // NOLINT(cppcoreguidelines-pro-type-const-cast)
  }

  inline void SetNodeAttr(const char* name, const char* value) {
    ASSERT(name);
    ASSERT(value);
    return SetAttr(AGNODE, name, value);
  }

  inline void SetGraphAttr(const char* name, const char* value) {
    ASSERT(name);
    ASSERT(value);
    return SetAttr(AGRAPH, name, value);
  }

  inline void SetEdgeAttr(const char* name, const char* value) {
    ASSERT(name);
    ASSERT(value);
    return SetAttr(AGEDGE, name, value);
  }

  inline auto NewNode(const char* name) -> Node* {
    ASSERT(name);
    return N<true>(name);
  }

  inline auto NewNode(const std::string& name) -> Node* {
    ASSERT(!name.empty());
    return NewNode(name.c_str());
  }

  inline auto GetNode(const char* name) -> Node* {
    ASSERT(name);
    return N<false>(name);
  }

  inline auto NewEdge(Node* from, Node* to, const char* name) -> Edge* {
    ASSERT(from);
    ASSERT(to);
    ASSERT(name);
    return E<true>(from, to, name);
  }

  inline auto GetEdge(Node* from, Node* to, const char* name) -> Edge* {
    ASSERT(from);
    ASSERT(to);
    ASSERT(name);
    return E<false>(from, to, name);
  }

  inline auto SetNodeLabel(Node* node, const char* value) -> int {
    return Set(node, "label", value);
  }

  inline auto SetNodeLabel(Node* node, const std::string& value) -> int {
    ASSERT(node);
    ASSERT(!value.empty());
    return SetNodeLabel(node, value.c_str());
  }

  inline auto SetNodeLabel(Node* node, const std::stringstream& value) -> int {
    ASSERT(node);
    return SetNodeLabel(node, value.str());
  }

  inline auto SetNodeXLabel(Node* node, const char* value) -> int {
    return Set(node, "xlabel", value);
  }

  inline auto SetNodeXLabel(Node* node, const std::string& value) -> int {
    ASSERT(node);
    ASSERT(!value.empty());
    return SetNodeXLabel(node, value.c_str());
  }

  inline auto SetNodeXLabel(Node* node, const std::stringstream& value) -> int {
    ASSERT(node);
    return SetNodeXLabel(node, value.str());
  }

  inline auto SetEdgeLabel(Edge* edge, const char* value) -> int {
    ASSERT(edge);
    ASSERT(value);
    return Set(edge, "label", value);
  }

  inline auto SetEdgeLabel(Edge* edge, const std::string& value) -> int {
    ASSERT(edge);
    ASSERT(!value.empty());
    return SetEdgeLabel(edge, value.c_str());
  }

 public:
  virtual ~GraphDecorator() = default;

  auto GetGraph() const -> Graph* {
    return graph_;
  }
};

class GraphBuilder : public GraphDecorator {
  DEFINE_NON_COPYABLE_TYPE(GraphBuilder);

 protected:
  static inline auto NewGraph(const char* name, Agdesc_t desc, Agdisc_t* disc = nullptr) -> Agraph_t* {
    ASSERT(name);
    const auto graph = agopen(const_cast<char*>(name), desc, disc);  // NOLINT(cppcoreguidelines-pro-type-const-cast)
    ASSERT(graph);
    return graph;
  }

 protected:
  explicit GraphBuilder(Agraph_t* graph) :
    GraphDecorator(graph) {}
  explicit GraphBuilder(const char* name, Agdesc_t desc = Agdirected) :
    GraphBuilder(NewGraph(name, desc)) {}

 public:
  virtual ~GraphBuilder() = default;
  virtual auto Build() -> dot::Graph* = 0;
};

class GraphRenderer {
  using Context = GVC_t;
  DEFINE_NON_COPYABLE_TYPE(GraphRenderer);

 private:
  Context* context_ = nullptr;

  void SetContext(Context* ctx) {
    ASSERT(ctx);
    context_ = ctx;
  }

  auto GetContext() const -> Context* {
    return context_;
  }

  auto HasContext() const -> bool {
    return GetContext() != nullptr;
  }

 private:
  static inline auto NewContext() -> Context* {
    return gvContext();
  }

  static inline void DeleteContext(Context* ctx) {
    ASSERT(ctx);
    gvFreeContext(ctx);
  }

 public:
  GraphRenderer() {
    SetContext(NewContext());
  }
  ~GraphRenderer() {
    if (HasContext())
      DeleteContext(GetContext());
  }

  void RenderTo(Graph* graph, FILE* stream, const std::string& layout, const std::string& format);
  void RenderDotTo(Graph* graph, FILE* stream);

  inline void RenderDotToStdout(Graph* graph) {
    return RenderDotTo(graph, stdout);
  }

  inline void RenderPngTo(Graph* graph, FILE* stream, const std::string& layout = "dot") {
    ASSERT(stream);
    ASSERT(graph);
    ASSERT(!layout.empty());
    return RenderTo(graph, stream, layout, "png");
  }
};
}  // namespace scm::dot

#endif  // SCM_GV_H
