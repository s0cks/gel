#include "scheme/rx.h"
#ifdef SCM_ENABLE_RX

#include "scheme/common.h"
#include "scheme/local_scope.h"
#include "scheme/thread_local.h"

namespace scm::rx {
static LazyThreadLocal<LocalScope> kRxScope([]() {
  return LocalScope::New();
});

auto GetRxScope() -> LocalScope* {
  return kRxScope;
}

// auto Observable::CreateClass() -> Class* {
//   return Class::New(Object::GetClass(), "Observable");
// }

// auto Observable::ToString() const -> std::string {
//   return "Observable()";
// }

// auto Observable::Equals(Object* rhs) const -> bool {
//   if (!rhs || !rhs->IsObservable())
//     return false;
//   NOT_IMPLEMENTED(FATAL);  // TODO: implement
//   return false;
// }

// auto Subscription::CreateClass() -> Class* {
//   return Class::New(Object::GetClass(), "Subscription");
// }

// auto Subscription::ToString() const -> std::string {
//   return "Observable()";
// }

// auto Subscription::Equals(Object* rhs) const -> bool {
//   if (!rhs || !rhs->IsSubscription())
//     return false;
//   NOT_IMPLEMENTED(FATAL);  // TODO: implement
//   return false;
// }
}  // namespace scm::rx

#endif  // SCM_ENABLE_RX