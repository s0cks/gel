#ifndef SCM_LOCAL_SCOPE_H
#define SCM_LOCAL_SCOPE_H

#include "scheme/common.h"
#include "scheme/local.h"
#include "scheme/pointer.h"

namespace scm {
class Symbol;
class LocalScope {
  friend class Repl;
  friend class Parser;
  friend class Runtime;
  using LocalList = std::vector<LocalVariable*>;
  DEFINE_NON_COPYABLE_TYPE(LocalScope);

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
  std::vector<LocalVariable*> locals_;

 protected:
  explicit LocalScope(LocalScope* parent = nullptr, const LocalList& locals = {}) :  // NOLINT(modernize-pass-by-value)
    parent_(parent),
    locals_(locals) {}

 public:
  virtual ~LocalScope() = default;

  virtual auto GetParent() const -> LocalScope* {
    return parent_;
  }

  inline auto HasParent() const -> bool {
    return GetParent() != nullptr;
  }

  inline auto IsRoot() const -> bool {
    return GetParent() == nullptr;
  }

  virtual auto Has(const std::string& name, const bool recursive = false) -> bool;
  virtual auto Has(const Symbol* symbol, const bool recursive = false) -> bool;
  virtual auto Add(LocalVariable* local) -> bool;
  auto Add(Symbol* symbol, Object* value = nullptr) -> bool;
  virtual auto Add(LocalScope* scope) -> bool;
  virtual auto Lookup(const std::string& name, LocalVariable** result, const bool recursive = true) -> bool;
  auto Lookup(const Symbol* symbol, LocalVariable** result, const bool recursive = true) -> bool;

  virtual auto GetLocalAt(const uword index) const -> LocalVariable* {
    ASSERT(index >= 0 && index <= GetNumberOfLocals());
    return locals_[index];
  }

  virtual auto IsEmpty() const -> bool {
    return locals_.empty();
  }

  virtual auto GetNumberOfLocals() const -> uint64_t {
    return locals_.size();
  }

  inline auto Add(const std::string& name, Object* value = nullptr) -> bool {
    ASSERT(!name.empty());

    LocalVariable* local = nullptr;
    if (!Lookup(name, &local, false))
      return Add(local = new LocalVariable(this, GetNumberOfLocals(), name, value));
    if (local->HasValue()) {
      DLOG(ERROR) << "cannot overwrite local: " << (*local);
      return false;
    }
    DLOG(INFO) << (*local) << " := " << value;
    local->SetValue(value);
    return true;
  }

  auto Accept(PointerVisitor* vis) -> bool;
  auto Accept(PointerPointerVisitor* vis) -> bool;
  auto VisitLocalPointers(const std::function<bool(Pointer**)>& vis, const bool recursive = true) -> bool;
  auto VisitLocals(const std::function<bool(Pointer*)>& vis, const bool recursive = true) -> bool;
  virtual auto VisitAllLocals(LocalVariableVisitor* vis) -> bool;
  virtual auto ToString() const -> std::string;

 public:
  static inline auto New(LocalScope* parent = nullptr) -> LocalScope* {
    return new LocalScope(parent);
  }

  static inline auto Union(const std::vector<LocalScope*>& scopes, LocalScope* parent = nullptr) -> LocalScope* {
    LocalList locals{};
    std::ranges::for_each(std::begin(scopes), std::end(scopes), [&locals](LocalScope* scope) {
      locals.insert(std::end(locals), std::begin(scope->locals_), std::end(scope->locals_));
    });
    return new LocalScope(parent, locals);
  }
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
#define PRINT_SCOPE(Severity, Scope) PRINT_SCOPE_AT_LEVEL(google::Severity, Scope)

}  // namespace scm

#endif  // SCM_LOCAL_SCOPE_H
