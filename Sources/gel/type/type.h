#ifndef GEL_TYPE_H
#define GEL_TYPE_H

#include <functional>
#include <vector>

#include "gel/common.h"
#include "gel/rx.h"

namespace gel {
#define FOR_EACH_TYPE(V) \
  V(Nil)                 \
  V(Str)                 \
  V(Set)                 \
  V(Pair)                \
  V(Bool)                \
  V(Macro)               \
  V(Object)              \
  V(Number)              \
  V(Fn)                  \
  V(InitFn)              \
  V(NativeFn)            \
  V(LambdaFn)

#define FORWARD_DECLARE(Name) class Name;
FOR_EACH_TYPE(FORWARD_DECLARE)
#undef FORWARD_DECLARE

using ObjectList = std::vector<Object*>;

#ifdef GEL_DEBUG
void PrintAllTypeSizes();
#endif  // GEL_DEBUG

#define DEFINE_TYPE_PREDICATE(Name) using Name##Predicate = std::function<bool(Name*)>;
FOR_EACH_TYPE(DEFINE_TYPE_PREDICATE);
#undef DEFINE_TYPE_PREDICATE

#define DEFINE_TYPE_VISITOR(Name)                    \
  class Name##Visitor {                              \
    DEFINE_NON_COPYABLE_TYPE(Name##Visitor);         \
                                                     \
   protected:                                        \
    Name##Visitor() = default;                       \
                                                     \
   public:                                           \
    virtual ~Name##Visitor() = default;              \
    virtual auto Visit##Name(Name* rhs) -> bool = 0; \
  };

// NOLINTBEGIN(cppcoreguidelines-special-member-functions)
FOR_EACH_TYPE(DEFINE_TYPE_VISITOR);
// NOLINTEND(cppcoreguidelines-special-member-functions)
#undef DEFINE_TYPE_VISITOR

}  // namespace gel

#endif  // GEL_TYPE_H
