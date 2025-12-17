#ifndef GEL_LOCAL_H
#define GEL_LOCAL_H

#include <cstdint>
#include <functional>
#include <ostream>
#include <string>

#include "gel/common.h"
#include "gel/heap/allocator.h"
#include "gel/heap/pointer.h"
#include "gel/type/type_traits.h"
#include "gel/type/value.h"

namespace gel {
class LocalVariable;
class PointerPointerVisitor;
class LocalVariableVisitor {
  DEFINE_NON_COPYABLE_TYPE(LocalVariableVisitor);

 protected:
  LocalVariableVisitor() = default;

 public:
  virtual ~LocalVariableVisitor() = default;
  virtual auto VisitLocal(LocalVariable* local) -> bool = 0;
};

class Str;
class LocalScope;
class LocalVariable : public HeapObject {
  friend class LocalScope;
  using Predicate = std::function<bool(LocalVariable*)>;

 public:
  static auto HasSymbol(Str* rhs) -> Predicate;
  static auto HasSymbol(const std::string& symbol) -> Predicate;

 private:
  LocalScope* owner_;
  uint64_t index_;
  Str* symbol_;
  Value* value_;

  LocalVariable(LocalScope* owner, uint64_t index, Str* symbol, Value* value) :
    owner_(owner),
    index_(index),
    symbol_(symbol),
    value_(value) {
    ASSERT(index_ >= 0);
    ASSERT(symbol_);
  }

  void SetOwner(LocalScope* scope) {
    ASSERT(scope);
    owner_ = scope;
  }

  void SetIndex(const uint64_t index) {
    index_ = index;
  }

  void SetSymbol(Str* rhs) {
    ASSERT(rhs);
    symbol_ = rhs;
  }

  auto VisitPointers(PointerVisitor* vis) -> bool override;
  auto VisitPointerPointers(PointerPointerVisitor* vis) -> bool override;

 public:
  ~LocalVariable() override = default;

  auto GetOwner() const -> LocalScope* {
    return owner_;
  }

  auto HasOwner() const -> bool {
    return GetOwner() != nullptr;
  }

  auto GetIndex() const -> uint64_t {
    return index_;
  }

  auto GetSymbol() const -> Str* {
    return symbol_;
  }

  inline auto HasSymbol() const -> bool {
    return GetSymbol() != nullptr;
  }

  auto GetValue() const -> Value*;
  void SetValue(Value* rhs);

  auto HasValue() const -> bool {
    return GetValue() != nullptr;
  }

  auto IsGlobal() const -> bool;
  auto ToString() const -> std::string override;

  friend auto operator<<(std::ostream& stream, const LocalVariable& rhs) -> std::ostream& {
    return stream << rhs.ToString();
  }

  DECLARE_HEAP_ALLOC_TYPE(LocalVariable);

 public:
  static inline auto New(LocalScope* owner, const uint64_t index, Str* symbol, Value* value = nullptr)
      -> LocalVariable* {
    ASSERT(owner);
    ASSERT(index >= 0);
    ASSERT(symbol);
    return new LocalVariable(owner, index, symbol, value);
  }

  static auto New(LocalScope* owner, Str* name, Value* value = nullptr) -> LocalVariable*;
  static auto New(LocalScope* owner, const std::string& name, Value* value = nullptr) -> LocalVariable*;
};
// TODO(@s0cks): implement
// static_assert(WithSymbol<LocalVariable>);
}  // namespace gel

#endif  // GEL_LOCAL_H
