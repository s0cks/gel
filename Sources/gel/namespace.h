#ifndef GEL_NAMESPACE_H
#define GEL_NAMESPACE_H

#include <vector>

#include "gel/common.h"
#include "gel/object.h"

namespace gel {
class Namespace : public Object {
  friend class Script;
  friend class Parser;

 private:
  Object* owner_ = nullptr;
  String* name_;
  LocalScope* scope_;
  String* docs_ = nullptr;

 protected:
  explicit Namespace(String* name, LocalScope* scope) :
    Object(),
    name_(name),
    scope_(scope) {
    ASSERT(name_);
    ASSERT(scope_);
  }

  void SetDocs(String* rhs) {
    ASSERT(rhs);
    docs_ = rhs;
  }

  void SetOwner(Object* rhs) {
    ASSERT(rhs);
    owner_ = rhs;
  }

 public:
  ~Namespace() override = default;

  auto GetName() const -> String* {
    return name_;
  }

  auto GetScope() const -> LocalScope* {
    return scope_;
  }

  auto GetDocs() const -> String* {
    return docs_;
  }

  auto GetOwner() const -> Object* {
    return owner_;
  }

  auto Prefix(Symbol* rhs) const -> Symbol*;
  auto HasPrefix(Symbol* rhs) const -> bool;
  DECLARE_TYPE(Namespace);

 public:
  static inline auto New(String* name, LocalScope* scope) -> Namespace* {
    ASSERT(name);
    return new Namespace(name, scope);
  }
};

using NamespaceList = std::vector<Namespace*>;
}  // namespace gel

#endif  // GEL_NAMESPACE_H
