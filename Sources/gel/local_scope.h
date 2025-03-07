#ifndef GEL_LOCAL_SCOPE_H
#define GEL_LOCAL_SCOPE_H

#include "gel/allocator.h"
#include "gel/array.h"
#include "gel/local.h"
#include "gel/pointer.h"

namespace gel {
class Symbol;
class LocalScope : public HeapObject {
  friend class Repl;
  friend class Parser;
  friend class Runtime;
  using LocalList = std::vector<LocalVariable*>;

 private:
  template <const bool Recursive>
  class IteratorBase {
    DEFINE_NON_COPYABLE_TYPE(IteratorBase);

   private:
    const LocalScope* scope_ = nullptr;
    uword index_ = 0;

   protected:
    auto GetIndex() const -> uword {
      return index_;
    }

    inline void IncrementIndex() {
      index_ += 1;
    }

    void SetScope(const LocalScope* scope) {
      ASSERT(scope);
      scope_ = scope;
    }

    void SetIndex(const uword index) {
      ASSERT(index >= 0);
      index_ = index;
    }

   public:
    explicit IteratorBase(const LocalScope* scope) {
      SetScope(scope);
    }
    virtual ~IteratorBase() = default;

    auto GetScope() const -> const LocalScope* {
      return scope_;
    }

    auto IsRecursive() const -> bool {
      return Recursive;
    }

    virtual auto HasNext() const -> bool = 0;
    virtual auto Next() -> LocalVariable* = 0;
  };

 public:
  class Iterator : public IteratorBase<false> {
    DEFINE_NON_COPYABLE_TYPE(Iterator);

   public:
    explicit Iterator(const LocalScope* scope) :
      IteratorBase<false>(scope) {}
    ~Iterator() override = default;

    auto HasNext() const -> bool override;
    auto Next() -> LocalVariable* override;
  };

  class RecursiveIterator : public IteratorBase<true> {
    DEFINE_NON_COPYABLE_TYPE(RecursiveIterator);

   private:
    inline void NextScope() {
      ASSERT(GetIndex() > GetScope()->GetNumberOfLocals());
      SetIndex(0);
      SetScope(GetScope()->GetParent());
    }

   public:
    explicit RecursiveIterator(const LocalScope* scope) :
      IteratorBase<true>(scope) {}
    ~RecursiveIterator() override = default;

    auto HasNext() const -> bool override;
    auto Next() -> LocalVariable* override;
  };

 private:
  LocalScope* parent_;
  Array<LocalVariable*>* locals_;

 protected:
  explicit LocalScope(LocalScope* parent, Array<LocalVariable*>* locals) :
    parent_(parent),
    locals_(locals) {}
  LocalScope() :
    LocalScope(nullptr, nullptr) {}

  auto VisitPointers(PointerVisitor* vis) -> bool override;
  auto VisitPointerPointers(PointerPointerVisitor* vis) -> bool override;

 public:
  ~LocalScope() override = default;

  virtual auto GetParent() const -> LocalScope* {
    return parent_;
  }

  inline auto HasParent() const -> bool {
    return GetParent() != nullptr;
  }

  inline auto IsRoot() const -> bool {
    return GetParent() == nullptr;
  }

  auto GetLocals() const -> Array<LocalVariable*>* {
    return locals_;
  }

  virtual auto GetLocalAt(const uword index) const -> LocalVariable* {
    ASSERT(locals_);
    ASSERT(index >= 0 && index <= GetNumberOfLocals());
    return locals_->Get(index);
  }

  virtual auto IsEmpty() const -> bool {
    ASSERT(locals_);
    return locals_->IsEmpty();
  }

  virtual auto GetNumberOfLocals() const -> uint64_t {
    ASSERT(locals_);
    return locals_->GetLength();
  }

  virtual auto Has(Symbol* rhs, const bool recursive = true) const -> bool;
  auto Has(const std::string& symbol, const bool recursive = true) const -> bool;

  inline auto HasLocal(Symbol* rhs) const -> bool {
    return Has(rhs, false);
  }

  auto HasLocal(const std::string& symbol) const -> bool;

  virtual auto Lookup(Symbol* symbol, LocalVariable** local, const bool recursive = true) const -> bool;
  auto Lookup(const std::string& symbol, LocalVariable** local, const bool recursive = true) const -> bool;

  virtual auto Add(LocalVariable* local) -> LocalVariable* {
    ASSERT(local);
    locals_->Push(local);
    return local;
  }

  auto Add(Symbol* symbol, Object* value = nullptr) -> LocalVariable*;
  auto Add(const std::string& symbol, Object* value = nullptr) -> LocalVariable*;

  inline auto AddThisValue(Object* rhs) -> LocalVariable* {
    ASSERT(rhs);
    return Add("this", rhs);
  }

  auto FindIf(const LocalVariable::Predicate& predicate) const -> LocalVariable* {
    return locals_->FindIf(predicate);
  }

  virtual void AddAll(LocalScope* rhs) {
    ASSERT(rhs);
    return locals_->AddAll(rhs->GetLocals());
  }

  template <class T>
  inline auto Add(T* value, std::enable_if_t<gel::is_object<T>::value && gel::has_symbol<T>::value>* = nullptr)
      -> LocalVariable* {
    ASSERT(value);
    return Add(value->GetSymbol(), value);
  }

  virtual auto VisitAllLocals(LocalVariableVisitor* vis, const bool recursive = false) -> bool;
  auto ToString() const -> std::string override;
  DECLARE_HEAP_ALLOC_TYPE(LocalScope);

 public:
  static inline auto New(LocalScope* parent = nullptr, Array<LocalVariable*>* locals = Array<LocalVariable*>::New())
      -> LocalScope* {
    return new LocalScope(parent, locals);
  }

  static inline auto NewWithThis(Object* this_value, LocalScope* parent = nullptr) -> LocalScope* {
    ASSERT(this_value);
    const auto scope = LocalScope::New();
    ASSERT(scope);
    const auto self = LocalVariable::New(scope, "this", this_value);
    LOG_IF(FATAL, !scope->Add(self)) << "failed to add " << (*self) << " to scope.";
    return scope;
  }

  static auto Union(const std::vector<LocalScope*>& scopes, LocalScope* parent = nullptr) -> LocalScope*;
};

class LocalScopeIterator {
  DEFINE_NON_COPYABLE_TYPE(LocalScopeIterator);

 private:
  LocalScope* scope_;

 public:
  explicit LocalScopeIterator(LocalScope* scope) :
    scope_(scope) {}
  ~LocalScopeIterator() = default;

  auto HasNext() const -> bool {
    return scope_ != nullptr;
  }

  auto Next() -> LocalScope* {
    const auto next = scope_;
    ASSERT(next);
    scope_ = next->GetParent();
    return next;
  }
};

class LocalScopePrinter : public LocalVariableVisitor {
  DEFINE_NON_COPYABLE_TYPE(LocalScopePrinter);

 private:
  std::string file_;
  int line_;
  google::LogSeverity severity_;
  bool recursive_;
  int indent_;

  auto GetIndentString() const -> std::string {
    return std::string(indent_ * 2, ' ');  // NOLINT(modernize-return-braced-init-list)
  }

  inline void Indent(const int by = 1) {
    ASSERT(by >= 1);
    indent_ += by;
  }

  inline void Deindent(const int by = 1) {
    ASSERT(by >= 1);
    indent_ -= by;
  }

 public:
  LocalScopePrinter(const char* file, const int line, const google::LogSeverity severity = google::INFO, const int indent = 0,
                    const bool recursive = true) :
    file_(file),
    line_(line),
    severity_(severity),
    recursive_(recursive),
    indent_(indent) {}
  ~LocalScopePrinter() override = default;

  auto GetFile() const -> const char* {
    return file_.c_str();
  }

  auto GetLine() const -> int {
    return line_;
  }

  auto GetSeverity() const -> google::LogSeverity {
    return severity_;
  }

  auto GetIndent() const -> int {
    return indent_;
  }

  auto IsRecursive() const -> bool {
    return recursive_;
  }

  auto VisitLocal(LocalVariable* local) -> bool override;
  auto PrintLocalScope(LocalScope* scope) -> bool;

 public:
  template <const google::LogSeverity Severity = google::INFO, const bool IsRecursive = true>
  static inline auto Print(LocalScope* scope, const char* file, const int line, const int indent = 0) -> bool {
    ASSERT(scope);
    ASSERT(file);
    LocalScopePrinter printer(file, line, Severity, indent, IsRecursive);
    return printer.PrintLocalScope(scope);
  }
};

#define PRINT_SCOPE_AT_LEVEL(Severity, Scope) LocalScopePrinter::Print<Severity>((Scope), __FILE__, __LINE__)
#define PRINT_SCOPE(Severity, Scope)          PRINT_SCOPE_AT_LEVEL(google::Severity, Scope)

}  // namespace gel

#endif  // GEL_LOCAL_SCOPE_H
