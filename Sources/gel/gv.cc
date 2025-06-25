#include "gel/gv.h"

#include <cstdio>
#include <glog/logging.h>
#include <graphviz/cgraph.h>
#include <graphviz/gvc.h>
#include <string>

#include "gel/common.h"

namespace gel::dot {
void SetGraphAttr(Graph* graph, const int kind, const char* name, const char* value) {
  ASSERT(name);
  ASSERT(value);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  agattr(graph, kind, const_cast<char*>(name), const_cast<char*>(value));
}

static inline auto N(Graph* graph, const char* name, const bool create) -> Node* {
  ASSERT(graph);
  ASSERT(name);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  return agnode(graph, const_cast<char*>(name), create);
}

auto NewNode(Graph* graph, const char* name) -> Node* {
  return N(graph, name, true);
}

auto GetNode(Graph* graph, const char* name) -> Node* {
  return N(graph, name, false);
}

static inline auto E(Graph* graph, const char* name, Node* from, Node* to, const bool create) -> Edge* {
  ASSERT(graph);
  ASSERT(from);
  ASSERT(to);
  ASSERT(name);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  return agedge(graph, from, to, const_cast<char*>(name), create);
}

auto NewEdge(Graph* graph, const char* name, Node* from, Node* to) -> Edge* {
  return E(graph, name, from, to, true);
}

auto GetEdge(Graph* graph, const char* name) -> Edge* {
  return E(graph, name, nullptr, nullptr, false);
}

auto DotGraph::New(GraphBuilder* builder) -> DotGraph* {
  ASSERT(builder);
  return new DotGraph(builder->GetGraph());
}

void GraphRenderer::RenderDotTo(Graph* graph, FILE* stream) {
  ASSERT(HasContext());
  ASSERT(graph);
  ASSERT(stream);
  return RenderTo(graph, stream, "dot", "dot");
}

void GraphRenderer::RenderTo(Graph* graph, FILE* stream, const std::string& layout, const std::string& format) {
  ASSERT(HasContext());
  ASSERT(stream);
  ASSERT(graph);
  ASSERT(!layout.empty());
  ASSERT(!format.empty());
  gvLayout(GetContext(), graph, layout.c_str());
  gvRender(GetContext(), graph, format.c_str(), stream);
  gvFreeLayout(GetContext(), graph);
}

void DotGraph::RenderTo(FILE* stream) {
  ASSERT(stream);
  GraphRenderer render;
  render.RenderDotTo(get(), stream);
}

void DotGraph::RenderPngTo(FILE* stream) {
  ASSERT(stream);
  GraphRenderer render;
  render.RenderPngTo(get(), stream);
}

void DotGraph::RenderPngToFilename(const std::string& filename) {
  ASSERT(!filename.empty());
  const auto file = fopen(filename.c_str(), "wb");
  LOG_IF(FATAL, !file) << "failed to open: " << filename;
  ASSERT(file);
  RenderPngTo(file);
  const auto result = fclose(file);
  LOG_IF(FATAL, result != 0) << "failed to close: " << filename;
}
}  // namespace gel::dot