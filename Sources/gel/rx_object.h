#include "gel/common.h"
#ifndef GEL_OBJECT_H
#error "Please #include <gel/object.h> instead."
#endif  // GEL_OBJECT_H

#ifndef GEL_RX_OBJECT_H
#define GEL_RX_OBJECT_H

#ifdef GEL_ENABLE_RX

#include "gel/hashcode.h"
#include "gel/object.h"
#include "gel/rx.h"

namespace gel {
class Observer : public Object {
 private:
  rx::DynamicObjectObserver value_;

 protected:
  explicit Observer(rx::DynamicObjectObserver value) :
    value_(value) {}

 public:
  ~Observer() override = default;

  auto get() const -> const rx::DynamicObjectObserver& {
    return value_;
  }

  DECLARE_TYPE(Observer);

 private:
  static auto CreateDynamicObserver(Procedure* on_next, Procedure* on_error, Procedure* on_completed)
      -> rx::DynamicObjectObserver;

 public:
  static auto New() -> Observer*;
  static inline auto New(Procedure* on_next, Procedure* on_error, Procedure* on_completed) -> Observer* {
    ASSERT(on_next);
    return new Observer(CreateDynamicObserver(on_next, on_error, on_completed));
  }

  static inline auto New(Object* on_next, Object* on_error, Object* on_completed) -> Observer* {
    ASSERT(on_next && on_next->IsProcedure());
    return New(gel::ToProcedure(on_next), gel::ToProcedure(on_error), gel::ToProcedure(on_completed));
  }
};

class Observable : public Object {
  friend class proc::rx_buffer;
  friend class proc::rx_map;
  friend class proc::rx_subscribe;

 private:
  rx::DynamicObjectObservable value_;

 private:
  explicit Observable(const rx::DynamicObjectObservable& value) :
    value_(value) {}

 public:
  ~Observable() override = default;

  auto get() const -> const rx::DynamicObjectObservable& {
    return value_;
  }

  // TODO: remove
  auto GetValue() const -> const rx::DynamicObjectObservable& {
    return value_;
  }

  template <typename O>
  void Apply(O&& op) {
    value_ = value_ | std::forward<O>(op);
  }

  template <typename S>
  void Subscribe(S&& on_next) {
    value_.subscribe(std::forward<S>(on_next));
  }

  void Subscribe(Observer* observer) {
    ASSERT(observer);
    get().subscribe(observer->get());
  }

  DECLARE_TYPE(Observable);

 public:
  static inline auto New(const rx::DynamicObjectObservable& value) -> Observable* {
    return new Observable(value);
  }

  static auto New(Object* value) -> Observable*;
  static auto Empty() -> Observable*;

 public:  //???
  static auto ToObservable(Pair* list) -> rx::DynamicObjectObservable;
};
}  // namespace gel

#endif  // GEL_ENABLE_RX
#endif  // GEL_RX_OBJECT_H
