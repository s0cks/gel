#ifndef SCM_SCRIPT_H
#define SCM_SCRIPT_H

#include "scheme/common.h"
#include "scheme/expression.h"
#include "scheme/lambda.h"
#include "scheme/local_scope.h"

namespace scm {
class Script : public Object {
  friend class Parser;
  friend class ScriptCompiler;
  using LambdaList = std::vector<Lambda*>;

 private:
  LocalScope* scope_;
  LambdaList lambdas_{};
  expr::ExpressionList body_{};
  instr::GraphEntryInstr* entry_ = nullptr;

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

  void SetEntry(instr::GraphEntryInstr* instr) {
    ASSERT(instr && !HasEntry());
    entry_ = instr;
  }

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

  auto GetEntry() const -> instr::GraphEntryInstr* {
    return entry_;
  }

  inline auto HasEntry() const -> bool {
    return GetEntry() != nullptr;
  }

  inline auto IsCompiled() const -> bool {
    return !IsEmpty() && HasEntry();
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
