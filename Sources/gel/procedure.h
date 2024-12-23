#ifndef GEL_PROCEDURE_H
#define GEL_PROCEDURE_H

#include "gel/common.h"
#include "gel/local_scope.h"
#include "gel/object.h"

namespace gel {
class Procedure : public Object {
  friend class Object;
  friend class Runtime;
  friend class Interpreter;
  DEFINE_NON_COPYABLE_TYPE(Procedure);

 private:
  Symbol* symbol_;

 protected:
  explicit Procedure(Symbol* symbol) :
    symbol_(symbol) {}

  void SetSymbol(Symbol* rhs) {
    ASSERT(rhs);
    symbol_ = rhs;
  }

  void RemoveSymbol() {
    symbol_ = nullptr;
  }

 public:
  ~Procedure() override = default;

  auto GetSymbol() const -> Symbol* {
    return symbol_;
  }

  inline auto HasSymbol() const -> bool {
    return GetSymbol() != nullptr;
  }

  auto HashCode() const -> uword override {
    uword hash = 0;
    if (HasSymbol())
      CombineHash(hash, GetSymbol()->Get());
    return hash;
  }

  auto Equals(Object* rhs) const -> bool override {
    return rhs && rhs->IsProcedure();
  }

  auto AsProcedure() -> Procedure* override {
    return this;
  }

  virtual auto IsNative() const -> bool {
    return false;
  }

  auto GetType() const -> Class* override {
    return GetClass();
  }

 private:
  static Class* kClass;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
  static auto CreateClass() -> Class*;
  static void InitClass();

 public:
  static inline auto New(const ObjectList& args) -> Procedure* {
    NOT_IMPLEMENTED(FATAL);
  }

  static auto operator new(const size_t sz) -> void*;
  static inline void operator delete(void* ptr) {
    ASSERT(ptr);
    // do nothing
  }
  static void Init();

  static inline auto GetClass() -> Class* {
    ASSERT(kClass);
    return kClass;
  }
};
}  // namespace gel

#endif  // GEL_PROCEDURE_H
