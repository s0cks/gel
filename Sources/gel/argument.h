#ifndef GEL_ARGUMENT_H
#define GEL_ARGUMENT_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

#include "common.h"
#include "object.h"
#include "platform.h"
#include "pointer.h"
#include "str.h"

namespace gel {
class Argument : public Object {
  friend class Object;
  DEFINE_NON_COPYABLE_TYPE(Argument);

 private:
  uint64_t index_;
  String* name_;
  bool optional_;
  bool vararg_;

  Argument(const uint64_t index, String* name, const bool optional, const bool vararg) :
    index_(index),
    name_(name),
    optional_(optional),
    vararg_(vararg) {
    ASSERT(name_);
    // TODO: assert that optional and vararg are either: false & false, false & true, true & false
  }

  auto VisitPointerPointers(PointerPointerVisitor* vis) -> bool override;

 public:
  ~Argument() override = default;

  auto GetType() const -> Class* override {
    return GetClass();
  }

  auto GetIndex() const -> uint64_t {
    return index_;
  }

  auto GetName() const -> String* {
    return name_;
  }

  inline auto HasName() const -> bool {
    return GetName() != nullptr;
  }

  auto IsOptional() const -> bool {
    return optional_;
  }

  auto IsVararg() const -> bool {
    return vararg_;
  }

  auto Compare(Object* rhs) const -> bool override;
  auto GetHashCode() const -> HashCode override;
  auto Equals(Object* rhs) const -> bool override;
  auto ToString() const -> std::string override;

 private:
  static Class* kClass;
  static auto CreateClass() -> Class*;
  static void InitClass();

 public:
  static inline auto GetClass() -> Class* {
    ASSERT(kClass);
    return kClass;
  }

  static auto operator new(const size_t sz) -> void*;
  static inline void operator delete(void* ptr) {
    ASSERT(ptr);
  }

  static inline auto New(const uint64_t idx, String* name, const bool optional, const bool vararg) -> Argument* {
    ASSERT(idx >= 0);
    ASSERT(name);
    return new Argument(idx, name, optional, vararg);
  }

  static inline auto New(const uint64_t idx, const std::string& name, const bool optional, const bool vararg)
      -> Argument* {
    ASSERT(idx >= 0);
    ASSERT(!name.empty());
    return new Argument(idx, String::New(name), optional, vararg);
  }
};
}  // namespace gel

#endif  // GEL_ARGUMENT_H
