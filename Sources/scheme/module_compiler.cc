#include "scheme/module_compiler.h"

#include <glog/logging.h>

#include "scheme/common.h"
#include "scheme/expression.h"
#include "scheme/flow_graph_builder.h"
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

  auto VisitModuleDef(expr::ModuleDefExpr* expr) -> bool override {
    NOT_IMPLEMENTED(FATAL);  // TODO: implement
    return false;
  }

  auto VisitLiteral(expr::LiteralExpr* expr) -> bool override {
    NOT_IMPLEMENTED(FATAL);  // TODO: implement
    return false;
  }

  auto VisitEval(expr::EvalExpr* expr) -> bool override {
    NOT_IMPLEMENTED(FATAL);  // TODO: implement
    return false;
  }

  auto VisitBegin(expr::BeginExpr* expr) -> bool override {
    NOT_IMPLEMENTED(FATAL);  // TODO: implement
    return false;
  }

  auto VisitLambda(expr::LambdaExpr* expr) -> bool override {
    NOT_IMPLEMENTED(FATAL);  // TODO: implement
    return false;
  }

  auto VisitCond(expr::CondExpr* expr) -> bool override {
    NOT_IMPLEMENTED(FATAL);  // TODO: implement
    return false;
  }

  auto VisitCallProc(expr::CallProcExpr* expr) -> bool override {
    NOT_IMPLEMENTED(FATAL);  // TODO: implement
    return false;
  }

  auto VisitSymbol(expr::SymbolExpr* expr) -> bool override {
    NOT_IMPLEMENTED(FATAL);  // TODO: implement
    return false;
  }

  auto VisitDefine(expr::DefineExpr* expr) -> bool override {
    ASSERT(expr);
    const auto symbol = expr->GetSymbol();
    ASSERT(symbol);
    if (GetScope()->Has(symbol, false)) {
      LOG(FATAL) << "cannot redefine: " << symbol;
      return false;
    }

    const auto value = expr->GetValue();
    ASSERT(value);
    if (!value->IsConstantExpr() && !value->IsLambda()) {
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

  auto VisitBinaryOp(expr::BinaryOpExpr* expr) -> bool override {
    NOT_IMPLEMENTED(FATAL);  // TODO: implement
    return false;
  }
};

auto ModuleCompiler::CompileModule(expr::ModuleDefExpr* expr) -> Module* {
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
  return Module::New(symbol, GetScope(), body);
}
}  // namespace scm