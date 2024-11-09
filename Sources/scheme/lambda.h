#ifndef SCM_LAMBDA_H
#define SCM_LAMBDA_H

#include <fmt/base.h>

#include <set>

#include "scheme/argument.h"
#include "scheme/common.h"
#include "scheme/expression.h"
#include "scheme/object.h"
#include "scheme/procedure.h"

namespace scm {
namespace expr {
class LambdaExpr;
class Expression;
}  // namespace expr
namespace instr {
class GraphEntryInstr;
}  // namespace instr

class Lambda : public Procedure {
  friend class LambdaCompiler;

 private:
  Object* owner_ = nullptr;
  Symbol* name_;
  ArgumentSet args_;
  expr::Expression* body_;
  instr::GraphEntryInstr* entry_ = nullptr;

  inline void SetEntry(instr::GraphEntryInstr* entry) {
    // ASSERT(entry && !IsCompiled());
    entry_ = entry;
  }

  void Apply() override;

 protected:
  Lambda(Symbol* name, ArgumentSet args, expr::Expression* body) :
    name_(name),
    args_(args),
    body_(body) {}

 public:
  ~Lambda() override = default;

  auto GetOwner() const -> Object* {
    return owner_;
  }

  inline auto HasOwner() const -> bool {
    return GetOwner() != nullptr;
  }

  void SetOwner(Object* rhs) {
    ASSERT(rhs);
    owner_ = rhs;
  }

  auto GetName() const -> Symbol* {
    return name_;
  }

  inline auto HasName() const -> bool {
    return GetName() != nullptr;
  }

  void SetName(Symbol* rhs) {
    ASSERT(rhs);
    name_ = rhs;
  }

  auto GetArgs() const -> const ArgumentSet& {
    return args_;
  }

  auto GetBody() const -> expr::Expression* {
    return body_;
  }

  inline auto HasBody() const -> bool {
    return GetBody() != nullptr;
  }

  inline auto IsEmpty() const -> bool {
    return !HasBody();
  }

  auto GetNumberOfArgs() const -> uint64_t {
    return args_.size();
  }

  auto GetEntry() const -> instr::GraphEntryInstr* {
    return entry_;
  }

  inline auto HasEntry() const -> bool {
    return GetEntry() != nullptr;
  }

  inline auto IsCompiled() const -> bool {
    return HasEntry();
  }

  auto GetType() const -> Class* override {
    return GetClass();
  }

  DECLARE_TYPE(Lambda);

 private:
  static Class* kClass;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

 public:
  static void Init();

  static inline auto GetClass() -> Class* {
    ASSERT(kClass);
    return kClass;
  }

  static inline auto New(Symbol* name, const ArgumentSet& args, expr::Expression* body) -> Lambda* {
    return new Lambda(name, args, body);
  }

  static inline auto New(const ArgumentSet& args, expr::Expression* body) -> Lambda* {
    return New(nullptr, args, body);
  }

  static inline auto New(Symbol* name, const ArgumentSet& args, const expr::ExpressionList& body) -> Lambda* {
    return New(args, expr::BeginExpr::New(body));  // TODO: remove
  }

  static inline auto New(const ArgumentSet& args, const expr::ExpressionList& body) -> Lambda* {
    return New(args, expr::BeginExpr::New(body));  // TODO: remove
  }
};

class LambdaCompiler {
  DEFINE_NON_COPYABLE_TYPE(LambdaCompiler);

 private:
  LocalScope* scope_;

 public:
  explicit LambdaCompiler(LocalScope* scope) :
    scope_(scope) {
    ASSERT(scope_);
  }
  virtual ~LambdaCompiler() = default;
  virtual auto CompileLambda(Lambda* lambda) -> bool;

  auto GetScope() const -> LocalScope* {
    return scope_;
  }

 public:
  static inline auto Compile(Lambda* lambda, LocalScope* scope) -> bool {
    ASSERT(lambda);
    ASSERT(scope);
    LambdaCompiler compiler(scope);
    return compiler.CompileLambda(lambda);
  }
};
}  // namespace scm

#endif  // SCM_LAMBDA_H
