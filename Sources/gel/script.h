#ifndef GEL_SCRIPT_H
#define GEL_SCRIPT_H

#include "gel/common.h"
#include "gel/expression.h"
#include "gel/lambda.h"
#include "gel/local_scope.h"

namespace gel {
class Script : public Object, public Executable {
  friend class Parser;
  friend class ScriptCompiler;
  using LambdaList = std::vector<Lambda*>;

 private:
  LocalScope* scope_;
  LambdaList lambdas_{};
  expr::ExpressionList body_{};

 protected:
  explicit Script(LocalScope* scope) :
    scope_(scope) {
    ASSERT(scope_);
  }

  inline void Append(expr::Expression* expr) {
    ASSERT(expr);
    body_.push_back(expr);
  }

  void Append(Lambda* lambda);

  auto VisitPointers(PointerVisitor* vis) -> bool override;

 public:
  ~Script() override = default;

  auto GetScope() const -> LocalScope* {
    return scope_;
  }

  auto GetBody() const -> const expr::ExpressionList& {
    return body_;
  }

  inline auto IsEmpty() const -> bool {
    return body_.empty();
  }

  DECLARE_TYPE(Script);

 public:
  static inline auto New(LocalScope* scope) -> Script* {
    ASSERT(scope);
    return new Script(scope);
  }

  static auto FromFile(const std::string& filename, const bool compile = true) -> Script*;
};

class ScriptCompiler {
  DEFINE_NON_COPYABLE_TYPE(ScriptCompiler);

 public:
  ScriptCompiler() = default;
  virtual ~ScriptCompiler() = default;
  virtual void CompileScript(Script* script);

 public:
  static void Compile(Script* script) {
    ASSERT(script);
    ScriptCompiler compiler;
    return compiler.CompileScript(script);
  }
};
}  // namespace gel

#endif  // GEL_SCRIPT_H
