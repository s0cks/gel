#include "scheme/lambda.h"

#include <glog/logging.h>

#include <ranges>
#include <sstream>

#include "scheme/common.h"
#include "scheme/expression.h"
#include "scheme/expression_compiler.h"
#include "scheme/flow_graph_builder.h"
#include "scheme/local_scope.h"
#include "scheme/runtime.h"

namespace scm {
Class* Lambda::kClass = nullptr;
void Lambda::Init() {
  ASSERT(kClass == nullptr);
  kClass = Class::New(Procedure::GetClass(), "Lambda");
  ASSERT(kClass);
}

auto Lambda::Equals(Object* rhs) const -> bool {
  if (!rhs->IsLambda())
    return false;
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return false;
}

void Lambda::Apply() {
  LOG_IF(FATAL, IsEmpty()) << "cannot compile Lambda w/ empty body.";
  const auto runtime = GetRuntime();
  const auto scope = runtime->GetCurrentScope();
  ASSERT(runtime);
  runtime->Call(GetEntry()->GetTarget(), scope);
}

auto Lambda::ToString() const -> std::string {
  std::stringstream ss;
  ss << "Lambda(";
  if (HasOwner())
    ss << "owner=" << GetOwner() << ", ";
  if (HasName())
    ss << "name=" << GetName()->Get() << ", ";
  ss << "args=" << GetArgs() << ", ";
  ss << "body=" << GetBody()->GetName();
  ss << ")";
  return ss.str();
}

auto LambdaCompiler::CompileLambda(Lambda* lambda) -> bool {
  ASSERT(lambda);
  const auto flow_graph = ExpressionCompiler::Compile(lambda->GetBody(), GetScope());
  if (flow_graph && flow_graph->HasEntry())
    lambda->SetEntry(flow_graph->GetEntry());
  return lambda->IsCompiled();
}
}  // namespace scm