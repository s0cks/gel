#ifndef GEL_RX_H
#define GEL_RX_H

#include <exception>
#include <rpp/observables/fwd.hpp>
#include <rpp/observers/dynamic_observer.hpp>
#include <rpp/sources/fwd.hpp>
#include <rpp/subjects/publish_subject.hpp>
#ifdef GEL_ENABLE_RX

#include <rpp/rpp.hpp>

#define FOR_EACH_RX_TYPE(V) \
  V(Observer)               \
  V(Observable)             \
  V(Subject)                \
  V(PublishSubject)         \
  V(ReplaySubject)

namespace gel {
class Object;
class Runtime;
class Procedure;
class LocalScope;
namespace rx {
using namespace rpp;
using DynamicObjectObservable = rx::dynamic_observable<Object*>;
using DynamicObjectObserver = rx::dynamic_observer<Object*>;
using PublishSubject = rx::subjects::publish_subject<Object*>;
using ReplaySubject = rx::subjects::replay_subject<Object*>;
using Predicate = std::function<bool(gel::Object*)>;
using MapFunc = std::function<gel::Object*(gel::Object*)>;
using OnCompleteFunc = std::function<void()>;
using OnNextFunc = std::function<void(gel::Object*)>;
using OnErrorFunc = std::function<void(std::exception_ptr)>;

static inline auto empty() -> DynamicObjectObservable {
  return source::empty<Object*>();
}

auto GetRxScope() -> LocalScope*;
auto CallPredicate(Runtime* runtime, Procedure* predicate) -> Predicate;
auto CallOnNext(Runtime* runtime, Procedure* proc) -> OnNextFunc;
auto CallOnError(Runtime* runtime, Procedure* proc) -> OnErrorFunc;
auto CallOnComplete(Runtime* runtime, Procedure* proc) -> OnCompleteFunc;
auto map(Runtime* runtime, Procedure* proc) -> rpp::operators::details::map_t<std::decay_t<MapFunc>>;

auto DoNothingOnNext() -> OnNextFunc;
auto DoNothingOnError() -> OnErrorFunc;
auto DoNothingOnComplete() -> OnCompleteFunc;
}  // namespace rx
}  // namespace gel

#else

#define FOR_EACH_RX_TYPE(V)

#endif  // GEL_ENABLE_RX
#endif  // GEL_RX_H
