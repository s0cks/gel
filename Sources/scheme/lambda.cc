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
Lambda::Lambda(const ArgumentSet& args, expr::LambdaExpr* expr) :
  Procedure(),
  args_(args),
  expr_(expr) {
  ASSERT(expr_);
}

auto Lambda::Equals(Object* rhs) const -> bool {
  if (!rhs->IsLambda())
    return false;
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return false;
}

auto Lambda::Apply(Runtime* runtime) -> bool {
  if (expr_) {
    compiled_ = ExpressionCompiler::Compile(expr::BeginExpr::New(expr_->GetBody()));
  }
  LOG_IF(FATAL, !compiled_) << "cannot exec uncompiled lambda: " << ToString();
  for (const auto& arg : std::ranges::reverse_view(GetArgs())) {
    const auto value = runtime->Pop();
    ASSERT(value);
    if (!runtime->GetScope()->Add(Symbol::New(arg.GetName()), value)) {
      LOG(ERROR) << "failed to define argument value.";
      GetRuntime()->PopScope();
      return false;
    }
  }

  runtime->PushScope();
  const auto result = runtime->Execute(compiled_->GetEntry());
  runtime->PopScope();
  if (!result)
    return true;
  runtime->Push(result);
  return true;
}

auto Lambda::ToString() const -> std::string {
  std::stringstream ss;
  ss << "Lambda(";
  ss << "args=" << GetArgs() << ", ";
  ss << "expr=" << GetExpression()->ToString();
  ss << ")";
  return ss.str();
}
}  // namespace scm