#ifndef GEL_TYPE_TRAITS_H
#define GEL_TYPE_TRAITS_H

#include <type_traits>

#include "gel/type.h"

namespace gel {
class FlowGraph;

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
#define DECLARE_IS_EXECUTABLE(Name) DECLARE_HAS_TRAIT(is_executable, Name)
DECLARE_IS_EXECUTABLE(Script);
DECLARE_IS_EXECUTABLE(Lambda);
DECLARE_IS_EXECUTABLE(Procedure);
DECLARE_IS_EXECUTABLE(NativeProcedure);
#undef DECLARE_IS_EXECUTABLE

DECLARE_TRAIT(has_entry);
#define DECLARE_HAS_ENTRY(Name) DECLARE_HAS_TRAIT(has_entry, Name)
DECLARE_HAS_ENTRY(Script);
DECLARE_HAS_ENTRY(Lambda);
DECLARE_HAS_ENTRY(Procedure);
DECLARE_HAS_ENTRY(FlowGraph);
DECLARE_HAS_ENTRY(NativeProcedure);
#undef DECLARE_HAS_ENTRY

DECLARE_TRAIT(is_iterable);
#define DECLARE_IS_ITERABLE(Name) DECLARE_HAS_TRAIT(is_iterable, Name)
DECLARE_IS_ITERABLE(Pair);

class ArrayBase;
DECLARE_IS_ITERABLE(ArrayBase);
#undef DECLARE_IS_ITERABLE

DECLARE_TRAIT(is_string_like);
#define DECLARE_IS_STRING_LIKE(Name) DECLARE_HAS_TRAIT(is_string_like, Name);
DECLARE_IS_STRING_LIKE(String);
DECLARE_IS_STRING_LIKE(Symbol);
#undef DECLARE_IS_STRING_LIKE

DECLARE_TRAIT(has_to_string);
#define DECLARE_HAS_TO_STRING(Name) DECLARE_HAS_TRAIT(has_to_string, Name);
DECLARE_HAS_TO_STRING(Object);
DECLARE_HAS_TO_STRING(expr::Expression);
DECLARE_HAS_TO_STRING(instr::Instruction);
DECLARE_HAS_TO_STRING(instr::Definition);
FOR_EACH_TYPE(DECLARE_HAS_TO_STRING)
#undef DECLARE_HAS_TO_STRING

DECLARE_TRAIT(has_docs);
#define DECLARE_HAS_DOCS(Name) DECLARE_HAS_TRAIT(has_docs, Name);
DECLARE_HAS_DOCS(Lambda);
DECLARE_HAS_DOCS(Namespace);
#undef DECLARE_HAS_DOCS

template <class T>
auto GetDocs(T* value, std::enable_if_t<has_docs<T>::value>* = nullptr) -> String*;

#undef DECLARE_HAS_TRAIT
#undef DECLARE_TRAIT
}  // namespace gel

#endif  // GEL_TYPE_TRAITS_H
