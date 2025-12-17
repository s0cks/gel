#ifndef GEL_BOOL_H
#define GEL_BOOL_H

#include "gel/common.h"
#include "gel/type/value.h"

namespace gel {
class Class;
class Bool : public Value {
 private:
  bool value_;

  explicit Bool(const bool value) :
    Value(),
    value_(value) {}

 public:
  ~Bool() override = default;

  auto Get() const -> bool {
    return value_;
  }

  auto Negate() const -> Bool* {
    return Get() ? False() : True();
  }

  DECLARE_VALUE_TYPE(Bool);

 private:
  static void Init();

 public:
  static auto New(const bool value) -> Bool*;

  static inline auto NewTrue() -> Bool* {
    return New(true);
  }

  static inline auto NewFalse() -> Bool* {
    return New(false);
  }

  static auto True() -> Bool*;
  static auto False() -> Bool*;

  static inline auto Box(const bool rhs) -> Bool* {
    return rhs ? True() : False();
  }

  static inline auto Unbox(Bool* rhs) -> bool {
    ASSERT(rhs);
    return rhs->Get();
  }
};

static inline auto Truth(gel::Value* rhs) -> bool {
  ASSERT(rhs);
  if (rhs->IsBool())
    return rhs->AsBool()->Get();
  return !rhs->IsNil();
}

static inline auto Not(Value* rhs) -> Value* {
  ASSERT(rhs);
  return Truth(rhs) ? Bool::False() : Bool::True();
}
}  // namespace gel

#endif  // GEL_BOOL_H
