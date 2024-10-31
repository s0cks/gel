#include "scheme/lambda.h"

#include <glog/logging.h>

#include <ranges>
#include <sstream>

#include "scheme/common.h"
#include "scheme/expression.h"
#include "scheme/expression_compiler.h"
#include "scheme/flow_graph_builder.h"
#include "scheme/runtime.h"

namespace scm {
auto Lambda::Equals(Type* rhs) const -> bool {
  if (!rhs->IsLambda())
    return false;
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return false;
}

auto Lambda::Apply(Runtime* runtime) const -> bool {
  const auto expr = ExpressionCompiler::Compile(GetBody());
  ASSERT(expr);

  RuntimeScopeScope scope(runtime);
  for (const auto& arg : std::ranges::reverse_view(GetArgs())) {
    const auto value = runtime->Pop();
    ASSERT(value);
    if (!scope->Add(Symbol::New(arg.GetName()), value)) {
      LOG(ERROR) << "failed to define argument value.";
      return false;
    }
  }

  const auto result = runtime->Execute(expr->GetEntry());
  if (!result) {
    DLOG(WARNING) << "no result from lambda.";
    return true;
  }
  runtime->Push(result);
  return true;
}

auto Lambda::ToString() const -> std::string {
  std::stringstream ss;
  ss << "Lambda(";
  ss << "args=" << GetArgs() << ", ";
  ss << "body=" << GetBody()->ToString();
  ss << ")";
  return ss.str();
}
}  // namespace scm