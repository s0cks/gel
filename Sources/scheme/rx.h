#ifndef SCM_RX_H
#define SCM_RX_H

#include <rpp/observables/fwd.hpp>
#include <rpp/observers/dynamic_observer.hpp>
#include <rpp/sources/fwd.hpp>
#include <rpp/subjects/publish_subject.hpp>
#ifdef SCM_ENABLE_RX

#include <rpp/rpp.hpp>

#define FOR_EACH_RX_TYPE(V) \
  V(Observer)               \
  V(Observable)             \
  V(Subject)                \
  V(PublishSubject)         \
  V(ReplaySubject)

namespace scm {
class Object;
class LocalScope;
namespace rx {
using namespace rpp;
using DynamicObjectObservable = rx::dynamic_observable<Object*>;
using DynamicObjectObserver = rx::dynamic_observer<Object*>;
using PublishSubject = rx::subjects::publish_subject<Object*>;
using ReplaySubject = rx::subjects::replay_subject<Object*>;

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
