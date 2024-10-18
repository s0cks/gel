#include "scheme/environment.h"

#include <glog/logging.h>

#include "scheme/common.h"

namespace scm {
auto Environment::Put(const std::string& k, Datum* v) -> bool {
  ASSERT(!k.empty());
  ASSERT(v);
  const auto [iter, success] = data_.insert({k, v});
  DLOG_IF(WARNING, !success) << "failed to put " << k << " := " << v->ToString() << " in environment.";
  return success;
}

auto Environment::Has(const std::string& k) const -> bool {
  ASSERT(!k.empty());
  auto env = this;
  while (env != nullptr) {
    const auto& data = env->data();
    const auto& found = data.find(k);
    if (found != std::end(data))
      return true;
    env = env->GetParent();
  }
  return false;
}

auto Environment::LocalLookup(const std::string& k, Datum** result) const -> bool {
  ASSERT(!k.empty());
  const auto& pos = data().find(k);
  const auto found = pos != std::end(data());
  DLOG_IF(WARNING, !found) << "couldn't find '" << k << "'";
  (*result) = found ? pos->second : nullptr;
  return found;
}

auto Environment::Lookup(const std::string& k, Datum** result) const -> bool {
  auto current = this;
  while (current != nullptr) {
    if (current->LocalLookup(k, result) && (*result) != nullptr)
      return true;
    current = current->GetParent();
  }
  (*result) = nullptr;
  return false;
}

auto Environment::New(Environment* parent) -> Environment* {
  return new Environment(parent);
}
}  // namespace scm