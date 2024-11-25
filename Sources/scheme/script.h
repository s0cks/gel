#ifndef SCM_SCRIPT_H
#define SCM_SCRIPT_H

#include "scheme/common.h"
#include "scheme/expression.h"
#include "scheme/lambda.h"
#include "scheme/local_scope.h"

namespace scm {
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

  auto GetType() const -> Class* override {
    return GetClass();
  }

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

 private:
  static Class* kClass;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

 public:
  static void Init();
  static inline auto New(LocalScope* scope) -> Script* {
    ASSERT(scope);
    return new Script(scope);
  }

  static inline auto GetClass() -> Class* {
    ASSERT(kClass);
    return kClass;
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
}  // namespace scm

#endif  // SCM_SCRIPT_H
