#ifndef SCM_ENVIRONMENT_H
#define SCM_ENVIRONMENT_H

#include <unordered_map>

#include "scheme/common.h"
#include "scheme/type.h"

namespace scm {
class Environment {
  DEFINE_NON_COPYABLE_TYPE(Environment);

 private:
  using Storage = std::unordered_map<std::string, Datum*>;

 public:
  class Iterator {
    DEFINE_NON_COPYABLE_TYPE(Iterator);

   private:
    const Environment* env_;
    Storage::const_iterator curr_;

   public:
    explicit Iterator(const Environment* env) :
      env_(env),
      curr_(env->begin()) {
      ASSERT(env);
    }
    ~Iterator() = default;

    auto GetEnv() const -> const Environment* {
      return env_;
    }

    auto HasNext() const -> bool {
      return curr_ != GetEnv()->end();
    }

    auto Next() -> Storage::value_type {
      return *(curr_++);
    }
  };

 private:
  Environment* parent_;
  Storage data_;

 protected:
  explicit Environment(Environment* parent = nullptr) :
    parent_(parent),
    data_() {}

  void SetParent(Environment* env) {
    ASSERT(env);
    parent_ = env;
  }

  auto data() const -> const Storage& {
    return data_;
  }

 public:
  ~Environment() = default;

  auto GetParent() const -> Environment* {
    return parent_;
  }

  auto begin() const -> Storage::const_iterator {
    return std::begin(data());
  }

  auto end() const -> Storage::const_iterator {
    return std::end(data());
  }

  auto Put(const std::string& k, Datum* v) -> bool;
  auto Has(const std::string& k) const -> bool;
  auto Lookup(const std::string& k, Datum** result) const -> bool;
  auto LocalLookup(const std::string& k, Datum** result) const -> bool;

  auto GetTotalNumberOfLocalValues() const -> uint64_t {
    return data().size();
  }

  auto GetTotalNumberOfValues() const -> uint64_t {
    uint64_t total = 0;
    auto current = this;
    while (current != nullptr) {
      total += current->GetTotalNumberOfLocalValues();
      current = current->GetParent();
    }
    return total;
  }

  auto IsEmpty() const -> bool {
    return data().empty();
  }

 public:
  static auto New(Environment* parent = nullptr) -> Environment*;
};
}  // namespace scm

#endif  // SCM_ENVIRONMENT_H