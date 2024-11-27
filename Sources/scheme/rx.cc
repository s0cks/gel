#include "scheme/rx.h"
#ifdef SCM_ENABLE_RX

#include "scheme/common.h"

namespace scm {

auto Observable::CreateClass() -> Class* {
  return Class::New(Object::GetClass(), "Observable");
}

auto Observable::ToString() const -> std::string {
  return "Observable()";
}

auto Observable::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsObservable())
    return false;
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto Subscription::CreateClass() -> Class* {
  return Class::New(Object::GetClass(), "Subscription");
}

auto Subscription::ToString() const -> std::string {
  return "Observable()";
}

auto Subscription::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsSubscription())
    return false;
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}
}  // namespace scm

#endif  // SCM_ENABLE_RX