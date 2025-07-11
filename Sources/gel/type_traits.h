#ifndef GEL_TYPE_TRAITS_H
#define GEL_TYPE_TRAITS_H

#include <concepts>
#include <memory>
#include <string>
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

template <class T>
concept IsGelObject = std::convertible_to<T, gel::Object>;

template <class T>
concept IsGelObjectPtr = std::convertible_to<T, gel::Object*>;

template <class T>
concept IsGelSymbol = std::same_as<T, gel::Symbol>;

template <class T>
concept IsGelSymbolPtr = std::same_as<T, gel::Symbol*>;

template <class T>
concept WithSymbol = requires(T value) {
  { value.GetSymbol() } -> std::convertible_to<Symbol*>;
  { value.HasSymbol() } -> std::convertible_to<bool>;
};

template <class T>
concept HasMutableSymbol = requires(T value) { value.SetSymbol((Symbol*)nullptr); };

template <typename T>
concept SymbolLike = std::same_as<T, std::string> || std::same_as<T, Symbol>;

template <WithSymbol Named, SymbolLike Sym>
static inline auto IsNamed(const Sym& name) -> std::function<bool(Named*)> {
  return [&](Named* value) {
    return value && value->GetSymbol()->Equals(name);
  };
}

template <class T>
concept HasDocstring = requires(T value) {
  { value.GetDocstring() } -> std::convertible_to<String*>;
  { value.HasDocstring() } -> std::convertible_to<bool>;
  { value.SetDocstring((String*)nullptr) };
};

DECLARE_TRAIT(is_iterable);
#define DECLARE_IS_ITERABLE(Name) DECLARE_HAS_TRAIT(is_iterable, Name)
DECLARE_IS_ITERABLE(Pair);

class ArrayBase;
DECLARE_IS_ITERABLE(ArrayBase);
#undef DECLARE_IS_ITERABLE

template <typename T>
concept StringLike =
    std::convertible_to<T, std::string> || std::same_as<T, gel::String> || std::same_as<T, gel::Symbol>;

template <typename T>
concept HasName = requires(T value) {
  { value.GetName() } -> std::convertible_to<gel::String*>;
  { value.HasName() } -> std::convertible_to<bool>;
};

template <typename T>
concept HasMutableName = requires(T value) { value.SetName((String*)nullptr); };

template <HasName Named, StringLike Str>
static inline auto IsNamed(const Str& name) -> std::function<bool(Named*)> {
  return [&](Named* value) {
    return value && value->GetName()->Equals(name);
  };
}

DECLARE_TRAIT(has_to_string);
#define DECLARE_HAS_TO_STRING(Name) DECLARE_HAS_TRAIT(has_to_string, Name);
DECLARE_HAS_TO_STRING(Object);
DECLARE_HAS_TO_STRING(expr::Expression);
DECLARE_HAS_TO_STRING(ir::Instruction);
DECLARE_HAS_TO_STRING(ir::Definition);
FOR_EACH_TYPE(DECLARE_HAS_TO_STRING)
#undef DECLARE_HAS_TO_STRING

DECLARE_TRAIT(has_docs);
#define DECLARE_HAS_DOCS(Name) DECLARE_HAS_TRAIT(has_docs, Name);
DECLARE_HAS_DOCS(Macro);
DECLARE_HAS_DOCS(Lambda);
DECLARE_HAS_DOCS(Namespace);
DECLARE_HAS_DOCS(NativeFn);
#undef DECLARE_HAS_DOCS

template <class T>
auto GetDocs(T* value, std::enable_if_t<has_docs<T>::value>* = nullptr) -> String*;

namespace ir {
class JoinEntryInstr;
class TargetEntryInstr;

DECLARE_TRAIT(is_entry);
#define DECLARE_IS_ENTRY(Name) DECLARE_HAS_TRAIT(is_entry, Name);
DECLARE_IS_ENTRY(JoinEntryInstr);
DECLARE_IS_ENTRY(TargetEntryInstr);
#undef DECLARE_IS_ENTRY
}  // namespace ir

#undef DECLARE_HAS_TRAIT
#undef DECLARE_TRAIT
}  // namespace gel

#endif  // GEL_TYPE_TRAITS_H
