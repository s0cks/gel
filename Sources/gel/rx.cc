#include "gel/rx.h"
#ifdef GEL_ENABLE_RX
#include <exception>
#include <rpp/operators/map.hpp>

#include "gel/common.h"
#include "gel/local_scope.h"
#include "gel/object.h"
#include "gel/procedure.h"
#include "gel/runtime.h"
#include "gel/thread_local.h"

namespace gel::rx {
static LazyThreadLocal<LocalScope> kRxScope([]() {
  return LocalScope::New();
});

auto GetRxScope() -> LocalScope* {
  return kRxScope;
}

auto CallPredicate(Runtime* runtime, Procedure* predicate) -> Predicate {
  ASSERT(runtime);
  ASSERT(predicate);
  return [runtime, predicate](gel::Object* value) {
    return gel::Truth(runtime->CallPop(predicate, {value}));
  };
}

auto DoNothingOnNext() -> OnNextFunc {
  static const OnNextFunc kDoNothing = [](gel::Object*) {
    // do nothing
  };
  return kDoNothing;
}

auto DoNothingOnError() -> OnErrorFunc {
  static const OnErrorFunc kDoNothing = [](std::exception_ptr exc) {
    // do nothing
  };
  return kDoNothing;
}

auto DoNothingOnComplete() -> OnCompleteFunc {
  static const OnCompleteFunc kDoNothing = []() {
    // do nothing
  };
  return kDoNothing;
}

auto CallOnNext(Runtime* runtime, Procedure* proc) -> OnNextFunc {
  ASSERT(runtime);
  if (gel::IsNull(proc))
    return DoNothingOnNext();
  ASSERT(proc);
  return [runtime, proc](gel::Object* next) {
    ASSERT(next);
    return runtime->Call(proc, {next});
  };
}

auto CallOnError(Runtime* runtime, Procedure* proc) -> OnErrorFunc {
  ASSERT(runtime);
  if (gel::IsNull(proc))
    return DoNothingOnError();
  ASSERT(proc);
  return [runtime, proc](std::exception_ptr error) -> void {
    try {
      std::rethrow_exception(error);
    } catch (const Exception& exc) {
      const auto error = Error::New(exc.what());
      return runtime->Call(proc, {error});
    }
  };
}

auto CallOnComplete(Runtime* runtime, Procedure* proc) -> OnCompleteFunc {
  ASSERT(runtime);
  if (gel::IsNull(proc))
    return DoNothingOnComplete();
  ASSERT(proc);
  return [runtime, proc]() {
    return runtime->Call(proc);
  };
}

auto map(Runtime* runtime, Procedure* proc) -> rpp::operators::details::map_t<std::decay_t<MapFunc>> {
  const MapFunc func = [runtime, proc](Object* value) {
    return runtime->CallPop(proc, {value});
  };
  return rpp::operators::map(func);
}
}  // namespace gel::rx

#endif  // GEL_ENABLE_RX