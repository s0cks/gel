#include "scheme/script.h"

#include <units.h>

#include "scheme/common.h"
#include "scheme/expression_dot.h"
#include "scheme/flags.h"
#include "scheme/flow_graph_builder.h"
#include "scheme/flow_graph_dot.h"
#include "scheme/lambda.h"

namespace scm {
Class* Script::kClass = nullptr;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
void Script::Init() {
  ASSERT(kClass == nullptr);
  kClass = Class::New(Object::GetClass(), "Script");
  ASSERT(kClass);
}

void Script::Append(Lambda* lambda) {
  ASSERT(lambda);
  lambdas_.push_back(lambda);
  lambda->SetOwner(this);
}

auto Script::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsScript())
    return false;
  const auto other = rhs->AsScript();
  ASSERT(other);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return true;
}

auto Script::ToString() const -> std::string {
  std::stringstream ss;
  ss << "Script(";
  ss << "scope=" << GetScope();
  ss << ")";
  return ss.str();
}

void ScriptCompiler::CompileScript(Script* script) {
  ASSERT(script && !script->IsCompiled());
#ifdef SCM_DEBUG
  DVLOG(SCM_VLEVEL_1) << "compiling: " << script->ToString();
  using Clock = std::chrono::high_resolution_clock;
  const auto start = Clock::now();
#endif  // SCM_DEBUG

#ifdef SCM_DEBUG
  if (FLAGS_dump_ast) {
    // TODO: dump script ast
  }
#endif  // SCM_DEBUG

  const auto flow_graph = FlowGraphBuilder::Build(script, script->GetScope());
  ASSERT(flow_graph);
  ASSERT(flow_graph->HasEntry());

#ifdef SCM_DEBUG
  if (FLAGS_dump_flow_graph) {
    const auto dotgraph = FlowGraphToDotGraph::BuildGraph("expr", flow_graph);
    ASSERT(dotgraph);
    dotgraph->RenderPngToFilename(GetReportFilename("exec_expr_flow_graph.png"));
  }
#endif  // SCM_DEBUG

#ifdef SCM_DEBUG
  const auto stop = Clock::now();
  const auto total_ns = std::chrono::duration_cast<std::chrono::milliseconds>((stop - start)).count();
  DVLOG(SCM_VLEVEL_1) << "script compiled in " << units::time::millisecond_t(total_ns);  // NOLINT
#endif                                                                                   // SCM_DEBUG
  script->SetEntry(flow_graph->GetEntry());
  // TODO: delete flow_graph
}
}  // namespace scm