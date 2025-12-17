#ifndef GEL_PAIR_H
#define GEL_PAIR_H

#include "gel/type/nil.h"
#include "gel/type/value.h"

namespace gel {
class Pair : public Value {
 private:
  Value* first_ = nullptr;
  Value* second_ = nullptr;

 protected:
  explicit Pair(Value* car = Nil::Get(), Value* cdr = Nil::Get()) :
    Value() {
    SetFirst(car);
    SetSecond(cdr);
  }

  auto VisitPointers(PointerVisitor* vis) -> bool override;

 public:
  ~Pair() override = default;

  auto GetFirst() const -> Value* {
    return first_;
  }

  inline auto HasFirst() const -> bool {
    return !GetFirst()->IsNil();
  }

  void SetFirst(Value* rhs) {
    ASSERT(rhs);
    first_ = rhs;
  }

  auto GetSecond() const -> Value* {
    return second_;
  }

  inline auto HasSecond() const -> bool {
    return !GetSecond()->IsNil();
  }

  void SetSecond(Value* rhs) {
    ASSERT(rhs);
    second_ = rhs;
  }

  inline auto IsEmpty() const -> bool {
    return !(HasFirst() && HasSecond());
  }

  auto IsTuple() const -> bool {
    return HasSecond() && !GetSecond()->IsPair();
  }

  DECLARE_VALUE_TYPE(Pair);

 public:
  static auto Empty() -> Pair*;
  static inline auto NewEmpty() -> Pair* {
    return new Pair(Nil::Get(), Nil::Get());
  }
  static inline auto New(Value* car, Value* cdr) -> Pair* {
    return new Pair(car, cdr);
  }

  static auto VisitEmptyPointerPointer(const std::function<bool(Pointer**)>& vis) -> bool;
  static auto VisitEmptyPointerPointer(PointerPointerVisitor* vis) -> bool;
};

static inline auto Cons(Value* lhs, Value* rhs) -> Pair* {
  return Pair::New(lhs, rhs);
}

// static inline auto ToList(const ObjectList& values, const bool reverse = false) -> Value* {
//   Value* result = Nil::Get();
//   if (reverse) {
//     for (const auto& next : std::ranges::reverse_view(values)) {
//       result = Pair::New(next, result);
//     }
//   } else {
//     for (const auto& next : values) {
//       result = Pair::New(next, result);
//     }
//   }
//   ASSERT(result);
//   return result;
// }
//
// template <class Iter>
// static inline auto ToList(Iter& iter) -> Value* {
//   Value* result = Nil::Get();
//   while (iter.HasNext()) {
//     const auto next = iter.Next();
//     ASSERT(next);
//     result = Pair::New(next, result);
//   }
//   return result;
// }
//
// auto ListFromRange(const uint64_t from, const uint64_t to) -> gel::Value*;
//
// template <class Iter, typename T>
// static inline auto ToList(Iter& iter, const std::function<Value*(T)>& map) -> Value* {
//   Value* result = Nil::Get();
//   while (iter.HasNext()) {
//     const auto next = iter.Next();
//     ASSERT(next);
//     result = Pair::New(map(next), result);
//   }
//   return result;
// }
//
// static inline auto Car(Value* rhs) -> Value* {
//   ASSERT(rhs && rhs->IsPair());
//   const auto value = rhs->AsPair()->GetFirst();
//   return value ? value : Nil::Get();
// }
//
// static inline auto Cdr(Value* rhs) -> Value* {
//   ASSERT(rhs && rhs->IsPair());
//   const auto value = rhs->AsPair()->GetSecond();
//   return value ? value : Nil::Get();
// }
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
