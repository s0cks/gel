#ifndef GEL_OBJECT_H
#define GEL_OBJECT_H

#include <exception>
#include <fmt/format.h>
#include <functional>
#include <numeric>
#include <ostream>
#include <ranges>
#include <rpp/observables/dynamic_observable.hpp>
#include <rpp/observers/dynamic_observer.hpp>
#include <rpp/observers/observer.hpp>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <utility>

#include "gel/allocator.h"
#include "gel/binary_op.h"
#include "gel/common.h"
#include "gel/hashcode.h"
#include "gel/platform.h"
#include "gel/region.h"
#include "gel/rx.h"
#include "gel/type.h"
#include "gel/type_traits.h"
#include "gel/unary_op.h"

namespace gel {
namespace proc {
class rx_map;
class rx_subscribe;
class rx_buffer;
}  // namespace proc

class Pointer;
class Object;
class PointerVisitor;
class Object : public HeapObject {
  friend class Macro;
  friend class Parser;
  friend class Module;
  friend class Pointer;
  friend class RefBase;
  friend class Fn;
  friend class Namespace;
  DEFINE_NON_COPYABLE_TYPE(Object)
 protected:
  Object() = default;

  template <typename T>
  static inline void CombineHash(uword& seed, const T& rhs) {
    std::hash<T> hasher;
    seed ^= hasher(rhs) + 0x9e3779b9 + (seed << 6) + (seed >> 2);  // NOLINT(cppcoreguidelines-avoid-magic-numbers)
  }

  auto FieldAddrAtOffset(const uword offset) const -> Object** {
    const auto address = ((uword)this) + offset;  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
    return ((Object**)address);                   // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  auto FieldAddr(Field* field) const -> Object**;

  virtual void AddChild(Object* rhs) {
    ASSERT(rhs);
    // do nothing
  }

 public:
  ~Object() override = default;
  virtual auto GetType() const -> Class* = 0;
  virtual auto GetHashCode() const -> HashCode = 0;
  virtual auto Equals(Object* rhs) const -> bool = 0;

#define DECLARE_BINARY_OP(Name) virtual auto Name(Object* rhs) const -> Object*;
  FOR_EACH_BINARY_OP(DECLARE_BINARY_OP)

  virtual auto Compare(Object* rhs) const -> bool = 0;

  auto GetField(Field* field) const -> Object* {
    ASSERT(field);
    return (*FieldAddr(field));
  }

  void SetField(Field* field, Object* rhs) {
    (*FieldAddr(field)) = rhs;
  }

  virtual auto IsLocal() const -> bool {
    return false;
  }

  virtual auto IsArgument() const -> bool {
    return false;
  }

  virtual auto IsArray() const -> bool {
    return false;
  }

  virtual auto IsAtom() const -> bool {
    return false;
  }

  virtual auto AsExpression() -> expr::Expression* {
    return nullptr;
  }

  virtual auto IsExpression() -> bool {
    return AsExpression() != nullptr;
  }

#define DEFINE_TYPE_CHECK(Name)      \
  virtual auto As##Name() -> Name* { \
    return nullptr;                  \
  }                                  \
  auto Is##Name() -> bool {          \
    return As##Name() != nullptr;    \
  }
  FOR_EACH_TYPE(DEFINE_TYPE_CHECK)
#undef DEFINE_TYPE_CHECK
  static constexpr const auto kClassName = "Object";

 private:
  static Class* kClass;
  static auto CreateClass() -> Class*;
  static void InitClass();

 public:
  static void Init();
  static auto VisitClassPointerPointer(PointerPointerVisitor* vis) -> bool;

  static inline auto VisitClassPointerPointer(const std::function<bool(Pointer**)>& func) -> bool {
    PointerPointerVisitorWrapper vis = func;
    return VisitClassPointerPointer(&vis);
  }

  static inline auto GetClass() -> Class* {
    ASSERT(kClass);
    return kClass;
  }
};

struct ObjectHasher {
  auto operator()(Object* rhs) const -> size_t {
    ASSERT(rhs);
    return rhs->GetHashCode();
  }
};

struct ObjectEquals {
  auto operator()(Object* lhs, Object* rhs) const -> bool {
    ASSERT(rhs);
    return lhs->Equals(rhs);
  }
};

struct ObjectComparator {
  auto operator()(Object* lhs, Object* rhs) const -> bool {
    return lhs->Compare(rhs);
  }
};

namespace ir {
class GraphEntryInstr;
}

static inline auto operator<<(std::ostream& stream, Object* rhs) -> std::ostream& {
  return stream << rhs->ToString();
}

#define DECLARE_TYPE(Name)                                                  \
  friend class Class;                                                       \
  friend class Object;                                                      \
  DEFINE_NON_COPYABLE_TYPE(Name)                                            \
 private:                                                                   \
  static Class* kClass;                                                     \
  static void InitClass();                                                  \
  static auto CreateClass() -> Class*;                                      \
                                                                            \
 public:                                                                    \
  static auto New(const ObjectList& args) -> Name*;                         \
  static constexpr const auto kClassName = #Name;                           \
  static auto operator new(const size_t sz)->void*;                         \
  static inline void operator delete(void* ptr) {                           \
    ASSERT(ptr);                                                            \
  }                                                                         \
  static inline auto GetClass() -> Class* {                                 \
    ASSERT(kClass);                                                         \
    return kClass;                                                          \
  }                                                                         \
  static auto VisitClassPointerPointer(PointerPointerVisitor* vis) -> bool; \
                                                                            \
 public:                                                                    \
  auto GetHashCode() const -> HashCode override;                            \
  auto Equals(Object* rhs) const -> bool override;                          \
  auto Compare(Object* rhs) const -> bool override;                         \
  auto GetType() const -> Class* override {                                 \
    return GetClass();                                                      \
  }                                                                         \
  auto ToString() const -> std::string override;                            \
  auto As##Name() -> Name* override {                                       \
    return this;                                                            \
  }

}  // namespace gel

namespace gel {
class Seq : public Object {
  friend class Object;
  DEFINE_NON_COPYABLE_TYPE(Seq);

 private:
 protected:
  Seq() = default;

 public:
  ~Seq() override = default;
  virtual auto IsEmpty() const -> bool = 0;

  auto GetHashCode() const -> HashCode override;
  auto Equals(Object* rhs) const -> bool override;

  auto GetType() const -> Class* override {
    return GetClass();
  }

  auto AsSeq() -> Seq* override {
    return this;
  }

  static auto New(const ObjectList& args) -> Seq*;
  static auto operator new(const size_t sz) -> void*;
  static inline void operator delete(void* ptr) {
    ASSERT(ptr);
  }

 private:
  static Class* kClass;
  static void InitClass();
  static auto CreateClass() -> Class*;

 public:
  static auto VisitClassPointerPointer(PointerPointerVisitor* vis) -> bool;

  static inline auto GetClass() -> Class* {
    ASSERT(kClass);
    return kClass;
  }
};

template <typename T>
auto PrintValue(std::ostream& stream, T& value) -> std::ostream&;

auto PrintValue(std::ostream& stream, Object* value) -> std::ostream&;

#define DEFINE_TYPE_PRED(Name)                       \
  static inline auto Is##Name(Object* rhs) -> bool { \
    return rhs && rhs->Is##Name();                   \
  }
FOR_EACH_TYPE(DEFINE_TYPE_PRED)
#undef DEFINE_TYPE_PRED

#define DEFINE_TYPE_CAST(Name)                                 \
  static inline auto To##Name(Object* rhs) -> Name* {          \
    return rhs && rhs->Is##Name() ? rhs->As##Name() : nullptr; \
  }
FOR_EACH_TYPE(DEFINE_TYPE_CAST)
#undef DEFINE_TYPE_CAST
}  // namespace gel

#endif  // GEL_OBJECT_H
