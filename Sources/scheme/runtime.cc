#include "scheme/runtime.h"

#include <glog/logging.h>

#include <utility>

#include "scheme/common.h"
#include "scheme/procedure.h"
#include "scheme/type.h"

namespace scm {
static inline auto IsSymbol(Datum* rhs) -> bool {
  return rhs && rhs->IsSymbol();
}

static inline auto IsSymbol(Datum* value, const std::string& rhs) -> bool {
  return IsSymbol(value) && value->AsSymbol()->Get() == rhs;
}

static inline auto Car(Pair* p) -> Datum* {
  ASSERT(p);
  return p->GetCar();
}

static inline auto Car(Datum* p) -> Datum* {
  ASSERT(p);
  return p->IsPair() ? Car(p->AsPair()) : Null::Get();
}

static inline auto Cdr(Pair* p) -> Datum* {
  ASSERT(p);
  return p->GetCdr();
}

static inline auto Cdr(Datum* p) -> Datum* {
  ASSERT(p);
  return p->IsPair() ? Cdr(p->AsPair()) : Null::Get();
}

class BinaryOp : public Procedure {
  using ApplyFunc = std::function<Datum*(Runtime*, Datum*, Datum*)>;
  DEFINE_NON_COPYABLE_TYPE(BinaryOp);

 private:
  std::string name_;
  ApplyFunc apply_;

 public:
  explicit BinaryOp(std::string name, ApplyFunc apply) :
    name_(std::move(name)),
    apply_(std::move(apply)) {}
  ~BinaryOp() override = default;

  auto GetName() const -> const std::string& {
    return name_;
  }

  auto Apply(Runtime* r, Datum* args) const -> Datum* override {
    Datum* result = Null::Get();
    auto next = args;
    while (next && !next->IsNull()) {
      if (next && next->IsPair()) {
        result = apply_(r, result, Car(next));
        next = Cdr(next);
      } else {
        result = apply_(r, result, next);
        next = Null::Get();
      }
    }
    return result;
  }

  auto ToString() const -> std::string override {
    std::stringstream ss;
    ss << "BinaryOp(";
    ss << "name=" << GetName();
    ss << ")";
    return ss.str();
  }
};
static const auto kAdd = BinaryOp("+", [](Runtime* r, Datum* lhs, Datum* rhs) {
  ASSERT(r);
  ASSERT(lhs);
  ASSERT(rhs);
  return r->Eval(lhs)->Add(r->Eval(rhs));
});
static const auto kSubtract = BinaryOp("-", [](Runtime* r, Datum* lhs, Datum* rhs) {
  ASSERT(r);
  ASSERT(lhs);
  ASSERT(rhs);
  return r->Eval(lhs)->Sub(r->Eval(rhs));
});
static const auto kMultiply = BinaryOp("*", [](Runtime* r, Datum* lhs, Datum* rhs) {
  ASSERT(r);
  ASSERT(lhs);
  ASSERT(rhs);
  return r->Eval(lhs)->Mul(r->Eval(rhs));
});
static const auto kDivide = BinaryOp("/", [](Runtime* r, Datum* lhs, Datum* rhs) {
  ASSERT(r);
  ASSERT(lhs);
  ASSERT(rhs);
  return r->Eval(lhs)->Div(r->Eval(rhs));
});

auto Runtime::EvalProc(Datum* fn, Datum* args, Environment* env) -> Datum* {
  if (IsSymbol(fn, "quote")) {
    return Car(args);
  } else if (IsSymbol(fn, "+")) {
    return kAdd.Apply(this, args);
  } else if (IsSymbol(fn, "-")) {
    return kSubtract.Apply(this, args);
  } else if (IsSymbol(fn, "*")) {
    return kMultiply.Apply(this, args);
  } else if (IsSymbol(fn, "/")) {
    return kDivide.Apply(this, args);
  }
  DLOG(WARNING) << "cannot eval proc: " << fn;
  return Null::Get();
}

auto Runtime::EvalSymbol(Symbol* sym, Environment* env) -> Datum* {
  ASSERT(sym);
  ASSERT(env);
  Datum* value = nullptr;
  if (!env->Lookup(sym->Get(), &value)) {
    LOG(ERROR) << "failed to find symbol '" << sym->Get() << "'";
    return Null::Get();
  }
  return value;
}

auto Runtime::Eval(Datum* value, Environment* env) -> Datum* {
  DLOG(INFO) << "evaluating: " << value->ToString();
  if (IsSymbol(value)) {
    return EvalSymbol(value->AsSymbol(), env);
  } else if (value->IsPair()) {
    return EvalProc(Car(value), Cdr(value), env);
  } else if (value->IsDatum()) {
    return value;
  }

  LOG(FATAL) << "invalid expression: " << value->ToString();
  return nullptr;
}

auto Runtime::New(Environment* globals) -> Runtime* {
  ASSERT(globals);
  const auto runtime = new Runtime(globals);
  ASSERT(runtime);
  return runtime;
}
}  // namespace scm