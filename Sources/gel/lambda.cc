#include "gel/lambda.h"

#include <glog/logging.h>

#include <ranges>
#include <sstream>

#include "gel/common.h"
#include "gel/expression.h"
#include "gel/expression_compiler.h"
#include "gel/flow_graph_builder.h"
#include "gel/local_scope.h"
#include "gel/pointer.h"
#include "gel/runtime.h"
#include "gel/type.h"

namespace gel {
auto Lambda::CreateClass() -> Class* {
  return Class::New(Procedure::GetClass(), kClassName);
}

auto Lambda::Equals(Object* rhs) const -> bool {
  if (!rhs->IsLambda())
    return false;
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return false;
}

auto Lambda::VisitPointers(PointerVisitor* vis) -> bool {
  ASSERT(vis);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto Lambda::New(const ObjectList& args) -> Lambda* {
  NOT_IMPLEMENTED(FATAL);
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
}  // namespace gel