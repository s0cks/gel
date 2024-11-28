#ifndef SCM_RX_H
#define SCM_RX_H

#include <rpp/observables/fwd.hpp>
#include <rpp/observers/dynamic_observer.hpp>
#include <rpp/sources/fwd.hpp>
#ifdef SCM_ENABLE_RX

#include <rpp/rpp.hpp>

#define FOR_EACH_RX_TYPE(V) \
  V(Observer)               \
  V(Observable)

namespace scm {
class Object;
class LocalScope;
namespace rx {
using namespace rpp;
using DynamicObjectObservable = rx::dynamic_observable<Object*>;
using DynamicObjectObserver = rx::dynamic_observer<Object*>;

static inline auto empty() -> DynamicObjectObservable {
  return source::empty<Object*>();
}

auto GetRxScope() -> LocalScope*;
}  // namespace rx
}  // namespace scm

#else

#define FOR_EACH_RX_TYPE(V)

#endif  // SCM_ENABLE_RX
#endif  // SCM_RX_H
