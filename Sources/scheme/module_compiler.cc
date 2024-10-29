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
auto DefinitionVisitor::VisitLocalDef(expr::LocalDef* expr) -> bool {
  ASSERT(expr);
  const auto symbol = expr->GetSymbol();
  ASSERT(symbol);
  if (GetScope()->Has(symbol, false)) {
    LOG(FATAL) << "cannot redefine: " << symbol;
    return false;
  }

  const auto value = expr->GetValue();
  ASSERT(value);
  DefinitionValueVisitor for_value(GetOwner());
  if (!expr->GetValue()->Accept(&for_value)) {
    LOG(FATAL) << "failed to visit: " << value;
    return false;
  }

  const auto local = LocalVariable::New(GetScope(), symbol, for_value.GetResult());
  ASSERT(local);
  if (!GetScope()->Add(local)) {
    LOG(FATAL) << "failed to define: " << (*local);
    return false;
  }

  DVLOG(10) << "defined module constant: " << (*local);
  return true;
}

auto DefinitionVisitor::VisitBinaryOpExpr(expr::BinaryOpExpr* expr) -> bool {
  ASSERT(expr);
  ASSERT(expr->IsConstantExpr());
  ReturnValue(expr->EvalToConstant());
  return true;
}

auto DefinitionVisitor::VisitModuleDef(expr::ModuleDef* expr) -> bool {
  for (auto idx = 0; idx < expr->GetNumberOfChildren(); idx++) {
    const auto defn = expr->GetDefinitionAt(idx);
    ASSERT(defn);
    DefinitionVisitor vis(GetOwner());
    if (!defn->Accept(&vis)) {
      LOG(FATAL) << "failed to visit module definition #" << idx << ": " << defn->ToString();
      return false;
    }
  }
  return true;
}

auto DefinitionVisitor::VisitImportDef(expr::ImportDef* expr) -> bool {
  ASSERT(expr);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto DefinitionVisitor::VisitLiteralExpr(expr::LiteralExpr* expr) -> bool {
  ASSERT(expr);
  ASSERT(expr->IsConstantExpr());
  ReturnValue(expr->EvalToConstant());
  return true;
}

auto DefinitionVisitor::VisitEvalExpr(expr::EvalExpr* expr) -> bool {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto DefinitionVisitor::VisitSetExpr(expr::SetExpr* expr) -> bool {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto DefinitionVisitor::VisitBeginExpr(expr::BeginExpr* expr) -> bool {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto DefinitionVisitor::VisitUnaryExpr(expr::UnaryExpr* expr) -> bool {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto DefinitionVisitor::VisitLambdaExpr(expr::LambdaExpr* expr) -> bool {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto DefinitionVisitor::VisitCondExpr(expr::CondExpr* expr) -> bool {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto DefinitionVisitor::VisitCallProcExpr(expr::CallProcExpr* expr) -> bool {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto DefinitionVisitor::VisitSymbolExpr(expr::SymbolExpr* expr) -> bool {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto DefinitionVisitor::VisitConsExpr(expr::ConsExpr* expr) -> bool {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto ModuleCompiler::CompileModule(expr::ModuleDef* expr) -> Module* {
  ASSERT(expr);
  DVLOG(10) << "compiling " << expr->ToString() << "....";
  const auto symbol = expr->GetSymbol();
  DefinitionVisitor vis(this);
  if (!expr->Accept(&vis)) {
    LOG(FATAL) << "failed to visit: " << expr->ToString();
    return nullptr;
  }

  if (FLAGS_dump_ast) {
    const auto dot_graph = expr::ExpressionToDot::BuildGraph(symbol, expr);
    ASSERT(dot_graph);
    dot_graph->RenderPngToFilename(GetReportFilename(fmt::format("module_{0:s}_ast.png", symbol->Get())));
  }
  // TODO: dump flow graph?
  return Module::New(symbol, GetScope());
}

auto DefinitionValueVisitor::VisitLambdaExpr(expr::LambdaExpr* expr) -> bool {
  ASSERT(expr);
  ReturnValue(Lambda::New(expr->GetArgs(), expr->GetBody()));
  return true;
}
}  // namespace scm