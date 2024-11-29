#ifndef SCM_TYPE_TRAITS_H
#define SCM_TYPE_TRAITS_H

#include "scheme/type.h"

namespace scm {
#define DECLARE_TRAIT(Name)                    \
  template <typename T>                        \
  struct Name {                                \
    static constexpr const auto value = false; \
  }

#define DECLARE_HAS_TRAIT(Trait, Type)        \
  template <>                                 \
  struct Trait<Type> {                        \
    static constexpr const auto value = true; \
  }

DECLARE_TRAIT(is_executable);
#define DECLARE_HAS_EXECUTABLE_TRAIT(Name) DECLARE_HAS_TRAIT(is_executable, Name)
DECLARE_HAS_EXECUTABLE_TRAIT(Script);
DECLARE_HAS_EXECUTABLE_TRAIT(Lambda);
DECLARE_HAS_EXECUTABLE_TRAIT(Procedure);
DECLARE_HAS_EXECUTABLE_TRAIT(NativeProcedure);
#undef DECLARE_HAS_EXECUTABLE_TRAIT

#undef DECLARE_HAS_TRAIT
#undef DECLARE_TRAIT
}  // namespace scm

#endif  // SCM_TYPE_TRAITS_H
