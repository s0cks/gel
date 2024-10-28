#include "scheme/module_compiler.h"

#include <glog/logging.h>

#include "scheme/common.h"
#include "scheme/expression.h"
#include "scheme/expression_dot.h"
#include "scheme/flags.h"
#include "scheme/flow_graph_builder.h"
#include "scheme/flow_graph_dot.h"
#include "scheme/module.h"

namespace scm {
class ModuleCompilerVisitor : public expr::ExpressionVisitor {
  DEFINE_NON_COPYABLE_TYPE(ModuleCompilerVisitor);

 private:
  ModuleCompiler* owner_;

  inline auto GetScope() const -> LocalScope* {
    return GetOwner()->GetScope();
  }

 public:
  explicit ModuleCompilerVisitor(ModuleCompiler* owner) :
    owner_(owner) {}
  ~ModuleCompilerVisitor() override = default;

  auto GetOwner() const -> ModuleCompiler* {
    return owner_;
  }

  auto VisitModuleDef(expr::ModuleDef* expr) -> bool override {
    NOT_IMPLEMENTED(FATAL);  // TODO: implement
    return false;
  }

  auto VisitLiteralExpr(expr::LiteralExpr* expr) -> bool override {
    NOT_IMPLEMENTED(FATAL);  // TODO: implement
    return false;
  }

  auto VisitEvalExpr(expr::EvalExpr* expr) -> bool override {
    NOT_IMPLEMENTED(FATAL);  // TODO: implement
    return false;
  }

  auto VisitBeginExpr(expr::BeginExpr* expr) -> bool override {
    NOT_IMPLEMENTED(FATAL);  // TODO: implement
    return false;
  }

  auto VisitLambdaExpr(expr::LambdaExpr* expr) -> bool override {
    NOT_IMPLEMENTED(FATAL);  // TODO: implement
    return false;
  }

  auto VisitCondExpr(expr::CondExpr* expr) -> bool override {
    NOT_IMPLEMENTED(FATAL);  // TODO: implement
    return false;
  }

  auto VisitCallProcExpr(expr::CallProcExpr* expr) -> bool override {
    NOT_IMPLEMENTED(FATAL);  // TODO: implement
    return false;
  }

  auto VisitSymbolExpr(expr::SymbolExpr* expr) -> bool override {
    NOT_IMPLEMENTED(FATAL);  // TODO: implement
    return false;
  }

  auto VisitLocalDef(expr::LocalDef* expr) -> bool override {
    ASSERT(expr);
    const auto symbol = expr->GetSymbol();
    ASSERT(symbol);
    if (GetScope()->Has(symbol, false)) {
      LOG(FATAL) << "cannot redefine: " << symbol;
      return false;
    }

    const auto value = expr->GetValue();
    ASSERT(value);
    if (!value->IsConstantExpr() && !value->IsLambdaExpr()) {
      LOG(FATAL) << "cannot define " << symbol << " as non-constant expression: " << value->ToString();
      return false;
    }

    const auto local = LocalVariable::New(GetScope(), symbol, value->EvalToConstant());
    ASSERT(local);
    if (!GetScope()->Add(local)) {
      LOG(FATAL) << "failed to define: " << (*local);
      return false;
    }

    DLOG(INFO) << "defined module constant: " << (*local);
    return true;
  }

  auto VisitBinaryOpExpr(expr::BinaryOpExpr* expr) -> bool override {
    NOT_IMPLEMENTED(FATAL);  // TODO: implement
    return false;
  }
};

auto ModuleCompiler::CompileModule(expr::ModuleDef* expr) -> Module* {
  ASSERT(expr);
  DLOG(INFO) << "compiling " << expr->ToString() << "....";
  const auto symbol = expr->GetSymbol();
  if (!expr->HasBody())
    return Module::New(symbol, GetScope());
  const auto body = expr->GetBody();
  ASSERT(body);
  ModuleCompilerVisitor vis(this);
  if (!body->Accept(&vis)) {
    LOG(FATAL) << "failed to visit: " << body->ToString();
    return nullptr;
  }
  if (FLAGS_dump_ast) {
    const auto dot_graph = expr::ExpressionToDot::BuildGraph(symbol, expr);
    ASSERT(dot_graph);
    dot_graph->RenderPngToFilename(GetReportFilename(fmt::format("module_{0:s}_ast.png", symbol->Get())));
  }
  // TODO: dump flow graph?
  return Module::New(symbol, GetScope(), body);
}
}  // namespace scm