#ifndef SCM_RUNTIME_H
#define SCM_RUNTIME_H

#include "scheme/common.h"
#include "scheme/environment.h"

namespace scm {
class Runtime {
  DEFINE_NON_COPYABLE_TYPE(Runtime);

 private:
  Environment* globals_ = nullptr;

  explicit Runtime(Environment* globals) {
    SetGlobals(globals);
  }

  void SetGlobals(Environment* env) {
    ASSERT(env);
    globals_ = env;
  }

  auto EvalSymbol(Symbol* sym, Environment* env) -> Datum*;
  auto EvalProc(Datum* fn, Datum* args, Environment* env = nullptr) -> Datum*;

 public:
  ~Runtime() = default;

  auto GetGlobals() const -> Environment* {
    return globals_;
  }

  auto Eval(Datum* value, Environment* env) -> Datum*;

  inline auto Eval(Datum* datum) -> Datum* {
    ASSERT(datum);
    return Eval(datum, GetGlobals());
  }

 public:
  static auto New(Environment* globals) -> Runtime*;
};
}  // namespace scm

#endif  // SCM_RUNTIME_H
