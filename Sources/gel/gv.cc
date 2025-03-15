#include "gel/gv.h"

#include <glog/logging.h>

namespace gel::dot {
void SetGraphAttr(Graph* graph, const int kind, const char* name, const char* value) {
  ASSERT(name);
  ASSERT(value);
  agattr(graph, kind, const_cast<char*>(name), const_cast<char*>(value));  // NOLINT(cppcoreguidelines-pro-type-const-cast)
}

static inline auto N(Graph* graph, const char* name, const bool create) -> Node* {
  ASSERT(graph);
  ASSERT(name);
  return agnode(graph, const_cast<char*>(name), create);  // NOLINT(cppcoreguidelines-pro-type-const-cast)
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
  return agedge(graph, from, to, const_cast<char*>(name), create);  // NOLINT(cppcoreguidelines-pro-type-const-cast)
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