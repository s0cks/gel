#ifndef GEL_ENVIRONMENT_H
#define GEL_ENVIRONMENT_H

#include "common.h"
#include "object.h"
#include "ref.h"
#include "symbol.h"

namespace gel {
class Environment {
  struct Hash {
    auto operator()(const Symbol* rhs) const -> size_t {
      return rhs->GetHashCode();
    }
  };

  struct Eq {
    auto operator()(const Symbol* lhs, const Symbol* rhs) const -> bool {
      return lhs->Equals(rhs->GetFullyQualifiedName());  // TODO @s0cks: fix this nonsense
    }
  };

  using Bindings = std::unordered_map<Symbol*, Object*, Hash, Eq>;
  DEFINE_NON_COPYABLE_TYPE(Environment);

 private:
  Environment* parent_ = nullptr;
  Bindings bindings_{};

  Environment() = default;

 public:
  ~Environment() = default;

  inline auto GetParent() const -> Environment* {
    return parent_;
  }

  inline auto HasParent() const -> bool {
    return GetParent() != nullptr;
  }

  inline auto IsGlobal() const -> bool {
    return GetParent() == nullptr;
  }

  void SetParent(Environment* rhs) {
    ASSERT(rhs);
    parent_ = rhs;
  }

  auto Get(Symbol* k) const -> Object* {
    ASSERT(k);
    const auto pos = bindings_.find(k);
    return pos != std::end(bindings_) ? pos->second : (Object*)nullptr;
  }

  inline auto Contains(Symbol* rhs) const -> bool {
    return Get(rhs) != nullptr;
  }

  auto Put(Symbol* k, Object* v) -> Object* {
    ASSERT(k);
    ASSERT(v);
    const auto [pos, success] = bindings_.insert({k, v});
    LOG_IF(FATAL, !success) << "failed to add binding " << k->ToString() << " to: " << v->ToString();
    return pos->second;
  }

  auto Remove(Symbol* k) -> bool {
    ASSERT(k);
    return bindings_.erase(k) == 1;
  }

  auto ToString() const -> std::string;
};
}  // namespace gel

#endif  // GEL_ENVIRONMENT_H
