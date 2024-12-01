#include "gel/rx.h"
#ifdef GEL_ENABLE_RX

#include "gel/common.h"
#include "gel/local_scope.h"
#include "gel/thread_local.h"

namespace gel::rx {
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
}  // namespace gel::rx

#endif  // GEL_ENABLE_RX