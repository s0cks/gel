#ifndef GEL_MACRO_COMPILER_H
#define GEL_MACRO_COMPILER_H

#include "gel/common.h"
#include "gel/expression.h"
#include "gel/macro.h"

namespace gel {
class MacroCompiler {
  DEFINE_NON_COPYABLE_TYPE(MacroCompiler);

 private:
  LocalScope* scope_;
  expr::MacroDef* macro_ = nullptr;

  inline void SetMacroDef(expr::MacroDef* expr) {
    ASSERT(expr);
    macro_ = expr;
  }

 public:
  explicit MacroCompiler(LocalScope* scope) :
    scope_(scope) {
    ASSERT(scope_);
  }
  ~MacroCompiler() = default;

  auto GetScope() const -> LocalScope* {
    return scope_;
  }

  auto GetMacroDef() const -> expr::MacroDef* {
    return macro_;
  }

  inline auto HasMacroDef() const -> bool {
    return GetMacroDef() != nullptr;
  }

  auto CompileMacro(expr::MacroDef* expr) -> Macro*;

 public:
  static inline auto Compile(expr::MacroDef* expr) -> Macro* {
    ASSERT(expr);
    // ASSERT(scope);
    // MacroCompiler compiler(scope);
    // return compiler.CompileMacro(expr);
    NOT_IMPLEMENTED(FATAL);  // TODO: implement
    return nullptr;
  }
};
}  // namespace gel

#endif  // GEL_MACRO_COMPILER_H
