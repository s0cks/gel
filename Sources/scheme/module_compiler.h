#ifndef SCM_MODULE_COMPILER_H
#define SCM_MODULE_COMPILER_H

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

  virtual auto CompileModule(expr::ModuleDefExpr* expr) -> Module*;

 public:
  static inline auto Compile(expr::ModuleDefExpr* expr) -> Module* {
    ASSERT(expr);
    ModuleCompiler compiler;
    return compiler.CompileModule(expr);
  }
};
}  // namespace scm

#endif  // SCM_MODULE_COMPILER_H
