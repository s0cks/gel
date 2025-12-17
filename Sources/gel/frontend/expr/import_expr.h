#ifndef GEL_IMPORT_EXPR_H
#define GEL_IMPORT_EXPR_H

#include "gel/frontend/expr/expr.h"

namespace gel::expr {
class Module;
class ImportExpr : public Expression {
 private:
  Module* module_;

  explicit ImportExpr(Module* module) :
    Expression(),
    module_(module) {
    ASSERT(module_);
  }

 public:
  ~ImportExpr() override = default;

  auto GetModule() const -> Module* {
    return module_;
  }

  DECLARE_EXPRESSION(ImportExpr);

 public:
  static inline auto New(Module* module) -> ImportExpr* {
    ASSERT(module);
    return new ImportExpr(module);
  }
};
}  // namespace gel::expr

#endif  // GEL_IMPORT_EXPR_H
