#ifndef SCM_MODULE_COMPILER_H
#define SCM_MODULE_COMPILER_H

#include "scheme/common.h"
#include "scheme/expression.h"

namespace scm {
class Module;
class ModuleCompiler {
  DEFINE_NON_COPYABLE_TYPE(ModuleCompiler);

 private:
  LocalScope* scope_ = LocalScope::New();

 protected:
  ModuleCompiler() = default;

  inline void SetScope(LocalScope* scope) {
    ASSERT(scope);
    scope_ = scope;
  }

 public:
  virtual ~ModuleCompiler() = default;

  auto GetScope() const -> LocalScope* {
    return scope_;
  }

  virtual auto CompileModule(ModuleDef* expr) -> Module*;

 public:
  static inline auto Compile(ModuleDef* expr) -> Module* {
    ASSERT(expr);
    ModuleCompiler compiler;
    return compiler.CompileModule(expr);
  }
};

class DefinitionVisitor : public expr::ExpressionVisitor {
  DEFINE_NON_COPYABLE_TYPE(DefinitionVisitor);

 private:
  ModuleCompiler* owner_;

 protected:
  inline auto GetScope() const -> LocalScope* {
    return GetOwner()->GetScope();
  }

  virtual void ReturnValue(Type* value) {}

 public:
  explicit DefinitionVisitor(ModuleCompiler* owner) :
    owner_(owner) {}
  ~DefinitionVisitor() override = default;

  auto GetOwner() const -> ModuleCompiler* {
    return owner_;
  }

  auto VisitModuleDef(expr::ModuleDef* expr) -> bool override;
  auto VisitLiteralExpr(expr::LiteralExpr* expr) -> bool override;
  auto VisitEvalExpr(expr::EvalExpr* expr) -> bool override;
  auto VisitBeginExpr(expr::BeginExpr* expr) -> bool override;
  auto VisitLambdaExpr(expr::LambdaExpr* expr) -> bool override;
  auto VisitCondExpr(expr::CondExpr* expr) -> bool override;
  auto VisitCallProcExpr(expr::CallProcExpr* expr) -> bool override;
  auto VisitSymbolExpr(expr::SymbolExpr* expr) -> bool override;
  auto VisitLocalDef(expr::LocalDef* expr) -> bool override;
  auto VisitBinaryOpExpr(expr::BinaryOpExpr* expr) -> bool override;
};

class DefinitionValueVisitor : public DefinitionVisitor {
  DEFINE_NON_COPYABLE_TYPE(DefinitionValueVisitor);

 private:
  Type* result_ = nullptr;

  void ReturnValue(Type* value) override {
    ASSERT(value);
    result_ = value;
  }

 public:
  explicit DefinitionValueVisitor(ModuleCompiler* owner) :
    DefinitionVisitor(owner) {}
  ~DefinitionValueVisitor() override = default;

  auto GetResult() const -> Type* {
    return result_;
  }

  inline auto HasResult() const -> bool {
    return GetResult() != nullptr;
  }

  auto VisitLambdaExpr(expr::LambdaExpr* expr) -> bool override;
};
}  // namespace scm

#endif  // SCM_MODULE_COMPILER_H
