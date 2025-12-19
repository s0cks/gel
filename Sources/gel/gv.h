#ifndef GEL_GV_H
#define GEL_GV_H

#ifdef GEL_GRAPHVIZ_ENABLED

#include <cstdio>
#include <fmt/format.h>
#include <glog/logging.h>
#include <graphviz/cgraph.h>
#include <graphviz/gvc.h>
#include <graphviz/gvcext.h>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#include "common.h"

namespace gel::dot {
using Symbol = Agsym_t;
using Graph = Agraph_t;
using Node = Agnode_t;
using NodeList = std::vector<Node*>;
using Edge = Agedge_t;
using EdgeList = std::vector<Edge*>;

template <typename T>
struct is_attr_value {
  static constexpr const auto value = false;
};

template <>
struct is_attr_value<const char*> {
  static constexpr const auto value = true;
};

template <>
struct is_attr_value<std::string> {
  static constexpr const auto value = true;
};

template <>
struct is_attr_value<std::stringstream> {
  static constexpr const auto value = true;
};

void SetGraphAttr(Graph* graph, const int kind, const char* name, const char* value);

static inline void SetGraphNodeAttr(Graph* graph, const char* name, const char* value) {
  ASSERT(name);
  ASSERT(value);
  return SetGraphAttr(graph, AGNODE, name, value);
}

static inline void SetGraphAttr(Graph* graph, const char* name, const char* value) {
  return SetGraphAttr(graph, AGRAPH, name, value);
}

static inline void SetGraphEdgeAttr(Graph* graph, const char* name, const char* value) {
  return SetGraphAttr(graph, AGEDGE, name, value);
}

template <typename T>
static inline auto SetProperty(T* obj, const char* name, const char* value) -> int {
  ASSERT(obj);
  ASSERT(name);
  ASSERT(value);
  return agset(obj, const_cast<char*>(name), value);  // NOLINT(cppcoreguidelines-pro-type-const-cast)
}

template <typename T>
static inline auto SetProperty(T* obj, const char* name, const std::string& value) -> int {
  ASSERT(!value.empty());
  return SetProperty<T>(obj, name, value.c_str());
}

template <typename T>
static inline auto SetProperty(T* obj, const char* name, const std::stringstream& value) -> int {
  return SetProperty<T>(obj, name, value.str());
}

auto NewNode(Graph* graph, const char* name) -> Node*;
auto GetNode(Graph* graph, const char* name) -> Node*;

template <typename V>
static inline auto SetNodeLabel(Node* node, const V& value, std::enable_if_t<is_attr_value<V>::value>* = nullptr)
    -> int {
  ASSERT(node);
  ASSERT(value);
  return SetProperty(node, "label", value);
}

template <typename V>
static inline auto SetNodeXLabel(Node* node, const V& value, std::enable_if_t<is_attr_value<V>::value>* = nullptr)
    -> int {
  return SetProperty(node, "xlabel", value);
}

auto NewEdge(Graph* graph, const char* name, Node* from, Node* to) -> Edge*;
auto GetEdge(Graph* graph, const char* name) -> Edge*;

template <typename V>
static inline auto SetEdgeLabel(Edge* edge, const V& value, std::enable_if_t<is_attr_value<V>::value>* = nullptr)
    -> int {
  ASSERT(edge);
  ASSERT(value);
  return SetProperty(edge, "label", value);
}

template <typename V>
static inline auto SetEdgeHeadlabel(Edge* edge, const V& value, std::enable_if_t<is_attr_value<V>::value>* = nullptr)
    -> int {
  return SetProperty(edge, "headlabel", value);
}

class GraphBuilder;
class DotGraph {
  using Handle = Agraph_t;
  DEFINE_NON_COPYABLE_TYPE(DotGraph);

 private:
  Handle* handle_ = nullptr;

  explicit DotGraph(Handle* handle) {
    SetHandle(handle);
  }

  inline void SetHandle(Handle* handle) {
    ASSERT(handle);
    handle_ = handle;
  }

 public:
  ~DotGraph();

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
  static inline auto New(Handle* handle) -> DotGraph* {
    return new DotGraph(handle);
  }

  static inline auto New(const char* name, Agdesc_t desc, Agdisc_t* disc = nullptr) -> DotGraph* {
    return New(agopen(const_cast<char*>(name), desc, disc));  // NOLINT
  }

  static inline auto New(const std::string& name, Agdesc_t desc, Agdisc_t* disc = nullptr) -> DotGraph* {
    return New(name.c_str(), desc, disc);
  }

  static auto New(GraphBuilder* builder) -> DotGraph*;
};

class GraphBuilder {
  friend class DotGraph;
  DEFINE_NON_COPYABLE_TYPE(GraphBuilder);

 private:
  Graph* graph_;

 protected:
  static inline auto NewGraph(const char* name, Agdesc_t desc, Agdisc_t* disc = nullptr) -> Agraph_t* {
    ASSERT(name);
    const auto graph = agopen(const_cast<char*>(name), desc, disc);  // NOLINT(cppcoreguidelines-pro-type-const-cast)
    ASSERT(graph);
    return graph;
  }

 protected:
  explicit GraphBuilder(Graph* graph) :
    graph_(graph) {
    ASSERT(graph_);
  }
  explicit GraphBuilder(const char* name, Agdesc_t desc = Agdirected) :
    GraphBuilder(NewGraph(name, desc)) {}

  auto GetGraph() const -> Graph* {
    return graph_;
  }

 public:
  virtual ~GraphBuilder() = default;
  virtual auto Build() -> dot::DotGraph* = 0;
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
}  // namespace gel::dot

#endif  // GEL_GRAPHVIZ_ENABLED

#endif  // GEL_GV_H
