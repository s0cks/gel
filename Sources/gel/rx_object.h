#ifndef GEL_OBJECT_H
#error "Please #include <gel/object.h> instead."
#endif  // GEL_OBJECT_H

#ifndef GEL_RX_OBJECT_H
#define GEL_RX_OBJECT_H

#ifdef GEL_ENABLE_RX

#include "gel/object.h"
#include "gel/rx.h"

namespace gel {
class Observer : public Instance {
 private:
  rx::DynamicObjectObserver value_;

 protected:
  explicit Observer(rx::DynamicObjectObserver value) :
    Instance(GetClass()),
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

class Observable : public Instance {
  friend class proc::rx_buffer;
  friend class proc::rx_map;
  friend class proc::rx_subscribe;

 private:
  rx::DynamicObjectObservable value_;

 private:
  explicit Observable(const rx::DynamicObjectObservable& value) :
    Instance(GetClass()),
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

class Subject : public Instance {
  friend class Object;
  DEFINE_NON_COPYABLE_TYPE(Subject);

 protected:
  explicit Subject(Class* type) :
    Instance(type) {}

  auto to_exception_ptr(Error* error) -> std::exception_ptr;

 public:
  ~Subject() override = default;

  auto Equals(Object* rhs) const -> bool override {
    return false;
  }

  auto AsSubject() -> Subject* override {
    return this;
  }

  virtual auto ToObservable() const -> Observable* = 0;
  virtual void Publish(Object* value) = 0;
  virtual void Complete() = 0;
  virtual void OnError(const std::exception_ptr& value) = 0;
  virtual void Subscribe(Observer* observer) = 0;
  virtual void Subscribe(const rx::OnNextFunc& on_next, const rx::OnErrorFunc& on_error,
                         const rx::OnCompleteFunc& on_completed) = 0;

  auto ToString() const -> std::string override;

 private:
  static constexpr const auto kClassName = "Subject";
  static Class* kClass;
  static auto CreateClass() -> Class*;
  static void InitClass();

 public:
  static auto New(const ObjectList& args) -> Subject*;
  static auto operator new(const size_t sz) -> void*;
  static inline void operator delete(void* ptr) {
    ASSERT(ptr);
  }

  static inline auto GetClass() -> Class* {
    ASSERT(kClass);
    return kClass;
  }
};

template <class S>
class TemplateSubject : public Subject {
  DEFINE_NON_COPYABLE_TYPE(TemplateSubject);

 private:
  S value_{};

 protected:
  explicit TemplateSubject(Class* type) :
    Subject(type) {}

 public:
  ~TemplateSubject() override = default;

  auto ToObservable() const -> Observable* override {
    return Observable::New(get().get_observable());
  }

  auto get() const -> const S& {
    return value_;
  }

  void OnError(const std::exception_ptr& value) override {
    ASSERT(value);
    get().get_observer().on_error(value);
  }

  void Publish(Object* value) override {
    ASSERT(value);
    if (value->IsError()) {
      return get().get_observer().on_error(to_exception_ptr(value->AsError()));
    } else {
      return get().get_observer().on_next(value);
    }
  }

  void Complete() override {
    get().get_observer().on_completed();
  }

  void Subscribe(Observer* observer) override {
    ASSERT(observer);
    get().get_observable().subscribe(observer->get());
  }

  void Subscribe(const std::function<void(Object*)>& on_next, const std::function<void(std::exception_ptr)>& on_error = {},
                 const std::function<void()>& on_completed = {}) override {
    get().get_observable().subscribe(on_next, on_error, on_completed);
  }
};

class PublishSubject : public TemplateSubject<rx::PublishSubject> {
 protected:
  PublishSubject() :
    TemplateSubject<rx::PublishSubject>(GetClass()) {}

 public:
  ~PublishSubject() override = default;

  DECLARE_TYPE(PublishSubject);

 public:
  static inline auto New() -> PublishSubject* {
    return new PublishSubject();
  }
};

class ReplaySubject : public TemplateSubject<rx::ReplaySubject> {
 protected:
  ReplaySubject() :
    TemplateSubject<rx::ReplaySubject>(GetClass()) {}

 public:
  ~ReplaySubject() override = default;
  DECLARE_TYPE(ReplaySubject);

 public:
  static inline auto New() -> ReplaySubject* {
    return new ReplaySubject();
  }
};
}  // namespace gel

#endif  // GEL_ENABLE_RX
#endif  // GEL_RX_OBJECT_H
