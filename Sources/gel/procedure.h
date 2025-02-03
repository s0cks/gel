#ifndef GEL_PROCEDURE_H
#define GEL_PROCEDURE_H

#include "gel/argument.h"
#include "gel/common.h"
#include "gel/local_scope.h"
#include "gel/object.h"
#include "gel/pointer.h"
#include "gel/symbol.h"
#include "gel/type_traits.h"

namespace gel {
class Procedure;
DECLARE_VISITOR(Procedure);
class Procedure : public Object {
  friend class Object;
  friend class Script;
  friend class Runtime;
  friend class Namespace;
  friend class Interpreter;
  DEFINE_NON_COPYABLE_TYPE(Procedure);

 public:
  using Predicate = std::function<bool(Procedure*)>;

  template <typename T>
  static inline auto IsNamed(T value, std::enable_if_t<gel::is_string_like<T>::value>* = nullptr) -> Predicate {
    return [value](Procedure* procedure) {
      ASSERT(procedure);
      return procedure->HasSymbol() && procedure->GetSymbol()->Equals(value);
    };
  }

 private:
  Symbol* symbol_;
  Object* owner_ = nullptr;
  String* docs_ = nullptr;
  Array<Argument*>* args_ = nullptr;

 protected:
  explicit Procedure(Symbol* symbol) :
    symbol_(symbol) {}

  void SetSymbol(Symbol* rhs) {
    ASSERT(rhs);
    symbol_ = rhs;
  }

  void SetOwner(Object* rhs) {
    ASSERT(rhs);
    owner_ = rhs;
  }

  void RemoveSymbol() {
    symbol_ = nullptr;
  }

  void RemoveOwner() {
    owner_ = nullptr;
  }

  void SetDocs(String* rhs) {
    ASSERT(rhs);
    docs_ = rhs;
  }

  void RemoveDocs() {
    docs_ = nullptr;
  }

  void SetArgs(Array<Argument*>* rhs) {
    ASSERT(rhs);
    args_ = rhs;
  }

  void RemoveArgs() {
    args_ = nullptr;
  }

  auto VisitPointers(PointerVisitor* vis) -> bool override {
    ASSERT(vis);
    if (HasSymbol()) {
      if (!vis->Visit(GetSymbol()))
        return false;
    }
    return true;
  }

  auto VisitPointerPointers(PointerPointerVisitor* vis) -> bool override {
    ASSERT(vis);
    if (!VisitPointerPointer(vis, &symbol_))
      return false;
    if (!VisitPointerPointer(vis, &owner_))
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
      CombineHash(hash, GetSymbol()->HashCode());
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

  auto GetOwner() const -> Object* {
    return owner_;
  }

  auto HasOwner() const -> bool {
    return GetOwner() != nullptr;
  }

  auto GetDocs() const -> String* {
    return docs_;
  }

  inline auto HasDocs() const -> bool {
    return GetDocs() != nullptr;
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

 private:
  static Class* kClass;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
  static auto CreateClass() -> Class*;
  static void InitClass();

 public:
  static auto VisitClassPointerPointer(PointerPointerVisitor* vis) -> bool;
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
