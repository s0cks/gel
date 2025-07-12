#ifndef GEL_PROCEDURE_H
#define GEL_PROCEDURE_H

#include "gel/argument.h"
#include "gel/common.h"
#include "gel/hashcode.h"
#include "gel/local_scope.h"
#include "gel/object.h"
#include "gel/pointer.h"
#include "gel/symbol.h"
#include "gel/type.h"
#include "gel/type_traits.h"

namespace gel {
namespace proto {
class IHaveOwner {
  DEFINE_NON_COPYABLE_TYPE(IHaveOwner);

 private:
  Object* owner_;

 protected:
  explicit IHaveOwner(Object* owner = nullptr) :
    owner_(owner) {}

  inline void SetOwner(Object* rhs) {
    owner_ = rhs;
  }

  inline void RemoveOwner() {
    return SetOwner(nullptr);
  }

 public:
  ~IHaveOwner() = default;

  auto GetOwner() const -> Object* {
    return owner_;
  }

  inline auto HasOwner() const -> bool {
    return GetOwner() != nullptr;
  }
};
}  // namespace proto

class Fn : public Object, public proto::IHaveOwner {
  friend class Object;
  friend class Script;
  friend class Runtime;
  friend class Namespace;
  friend class Interpreter;
  DEFINE_NON_COPYABLE_TYPE(Fn);

 public:
  template <StringLike Str>
  static inline auto IsNamed(const Str& value) -> FnPredicate {
    return [&](Fn* procedure) {
      ASSERT(procedure);
      return procedure->HasSymbol() && procedure->GetSymbol()->Equals(value);
    };
  }

 private:
  Symbol* symbol_;
  String* docs_ = nullptr;
  Array<Argument*>* args_ = nullptr;

 protected:
  explicit Fn(Symbol* symbol) :
    symbol_(symbol) {}

  void SetArgs(Array<Argument*>* rhs) {
    ASSERT(rhs);
    args_ = rhs;
  }

  void RemoveArgs() {
    args_ = nullptr;
  }

  auto VisitPointers(PointerVisitor* vis) -> bool override {
    ASSERT(vis);
    if (!Visit(symbol_, *vis))
      return false;
    if (!Visit(docs_, *vis))
      return false;
    if (!Visit(args_, *vis))
      return false;
    return true;
  }

  auto VisitPointerPointers(PointerPointerVisitor* vis) -> bool override {
    ASSERT(vis);
    if (!VisitPointerPointer(vis, &symbol_))
      return false;
    if (!VisitPointerPointer(vis, &docs_))
      return false;
    if (!VisitPointerPointer(vis, &args_))
      return false;
    return true;
  }

  void SetArgAt(const uint64_t idx, Argument* arg) {
    ASSERT(args_ && idx >= 0 && idx <= GetNumberOfArgs());
    ASSERT(arg);
    args_->Set(idx, arg);
  }

 public:
  ~Fn() override = default;

  auto GetSymbol() const -> Symbol* {
    return symbol_;
  }

  inline auto HasSymbol() const -> bool {
    return GetSymbol() != nullptr;
  }

  void SetSymbol(Symbol* rhs) {
    ASSERT(rhs);
    symbol_ = rhs;
  }

  void RemoveSymbol() {
    symbol_ = nullptr;
  }

  auto GetHashCode() const -> HashCode override {
    HashCode hash{};
    if (HasSymbol())
      hash ^= (*symbol_);
    return hash;
  }

  auto Equals(Object* rhs) const -> bool override {
    return rhs && rhs->IsFn();
  }

  auto AsFn() -> Fn* override {
    return this;
  }

  virtual auto IsNative() const -> bool {
    return false;
  }

  auto GetType() const -> Class* override {
    return GetClass();
  }

  auto HasOwner() const -> bool {
    return GetOwner() != nullptr;
  }

  auto GetDocstring() const -> String* {
    return docs_;
  }

  inline auto HasDocstring() const -> bool {
    return GetDocstring() != nullptr;
  }

  void SetDocstring(String* rhs) {
    ASSERT(rhs);
    docs_ = rhs;
  }

  void RemoveDocstring() {
    docs_ = nullptr;
  }

  auto GetArgs() const -> Array<Argument*>* {
    return args_;
  }

  inline auto GetNumberOfArgs() const -> uword {
    return GetArgs() ? GetArgs()->GetLength() : 0;
  }

  inline auto HasArgs() const -> bool {
    return GetNumberOfArgs() > 0;
  }

  auto GetArgAt(const uint64_t idx) const -> Argument* {
    ASSERT(args_ && idx >= 0 && idx <= GetNumberOfArgs());
    return args_->Get(idx);
  }

  auto HasArgAt(const uint64_t idx) const -> bool {
    return args_ && (idx >= 0 && idx <= GetNumberOfArgs()) && (args_->Get(idx) != nullptr);
  }

  inline friend auto operator<<(std::ostream& stream, const Fn& rhs) -> std::ostream& {
    return stream << rhs.ToString();
  }

 private:
  static Class* kClass;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
  static auto CreateClass() -> Class*;
  static void InitClass();

 public:
  static auto VisitClassPointerPointer(PointerPointerVisitor* vis) -> bool;
  static inline auto New(const ObjectList& args) -> Fn* {
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

 public:
  static inline auto IsLambdaProc(Fn* p) -> bool {
    return p && p->IsLambda();
  }

  static inline auto IsNativeProc(Fn* p) -> bool {
    return p && p->IsNative();
  }
};
static_assert(WithSymbol<Fn>);
static_assert(HasDocstring<Fn>);
}  // namespace gel

#endif  // GEL_PROCEDURE_H
