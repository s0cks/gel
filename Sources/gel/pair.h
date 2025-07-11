#ifndef GEL_PAIR_H
#define GEL_PAIR_H

#include "gel/nil.h"
#include "gel/object.h"

namespace gel {

class Pair : public Seq {
 public:
  static Field* kFirstField;
  static Field* kSecondField;

 protected:
  explicit Pair(Object* car = nullptr, Object* cdr = nullptr) :
    Seq() {
    if (car)
      SetFirst(car);
    if (cdr)
      SetSecond(cdr);
  }

  auto VisitPointers(PointerVisitor* vis) -> bool override;

 public:
  ~Pair() override = default;

  auto GetFirst() const -> Object* {
    return GetField(kFirstField);
  }

  inline auto HasFirst() const -> bool {
    return GetFirst() != nullptr;
  }

  void SetFirst(Object* rhs) {  // TODO: reduce visibility
    ASSERT(rhs);
    SetField(kFirstField, rhs);
  }

  auto GetSecond() const -> Object* {
    return GetField(kSecondField);
  }

  inline auto HasSecond() const -> bool {
    return GetSecond() != nullptr;
  }

  void SetSecond(Object* rhs) {  // TODO: reduce visibility
    ASSERT(rhs);
    SetField(kSecondField, rhs);
  }

  auto IsEmpty() const -> bool override {
    return !HasFirst() && !HasSecond();
  }

  auto IsTuple() const -> bool {
    return HasSecond() && !GetSecond()->IsPair();
  }

  DECLARE_TYPE(Pair);

 public:
  static auto Empty() -> Pair*;
  static inline auto NewEmpty() -> Pair* {
    return new Pair();
  }
  static inline auto New(Object* car, Object* cdr) -> Pair* {
    return new Pair(car, cdr);
  }

  static auto VisitEmptyPointerPointer(const std::function<bool(Pointer**)>& vis) -> bool;
  static auto VisitEmptyPointerPointer(PointerPointerVisitor* vis) -> bool;
};

static inline auto Cons(Object* lhs, Object* rhs) -> Object* {
  ASSERT(lhs);
  ASSERT(rhs);
  return Pair::New(lhs, rhs);
}

static inline auto ToList(const ObjectList& values, const bool reverse = false) -> Object* {
  Object* result = Nil::Get();
  if (reverse) {
    for (const auto& next : std::ranges::reverse_view(values)) {
      result = Pair::New(next, result);
    }
  } else {
    for (const auto& next : values) {
      result = Pair::New(next, result);
    }
  }
  ASSERT(result);
  return result;
}

template <class Iter>
static inline auto ToList(Iter& iter) -> Object* {
  Object* result = Nil::Get();
  while (iter.HasNext()) {
    const auto next = iter.Next();
    ASSERT(next);
    result = Pair::New(next, result);
  }
  return result;
}

auto ListFromRange(const uint64_t from, const uint64_t to) -> gel::Object*;

template <class Iter, typename T>
static inline auto ToList(Iter& iter, const std::function<Object*(T)>& map) -> Object* {
  Object* result = Nil::Get();
  while (iter.HasNext()) {
    const auto next = iter.Next();
    ASSERT(next);
    result = Pair::New(map(next), result);
  }
  return result;
}

static inline auto Car(Object* rhs) -> Object* {
  ASSERT(rhs && rhs->IsPair());
  const auto value = rhs->AsPair()->GetFirst();
  return value ? value : Nil::Get();
}

static inline auto Cdr(Object* rhs) -> Object* {
  ASSERT(rhs && rhs->IsPair());
  const auto value = rhs->AsPair()->GetSecond();
  return value ? value : Nil::Get();
}
}  // namespace gel

namespace fmt {
template <>
struct formatter<gel::Pair> : public formatter<std::string> {
  template <typename FormatContext>
  constexpr auto format(const gel::Pair& value, FormatContext& ctx) const -> decltype(ctx.out()) {
    ctx.out() << "(";
    if (value.IsEmpty()) {
      ctx.out() << ")";
      return ctx.out();
    }
    format_to(ctx.out(), "{}", *(value.GetFirst()));
    auto next = value.GetSecond();
    do {
      if (next->IsNil()) {
        ctx.out() << ")";
        return ctx.out();
      }
      if (!next->IsPair()) {
        ctx.out() << " ";
        format_to(ctx.out(), "{}", (*next));
        ctx.out() << ")";
        return ctx.out();
      }
      ctx.out() << " ";
      format_to(ctx.out(), "{}", next->AsPair()->GetFirst());
      next = next->AsPair()->GetSecond();
    } while (true);
  }
};
}  // namespace fmt

#endif  // GEL_PAIR_H
