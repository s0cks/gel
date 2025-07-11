#ifndef GEL_SUBJECT_H
#define GEL_SUBJECT_H

#include "gel/object.h"

namespace gel {
class Subject : public Object {
  friend class Object;
  DEFINE_NON_COPYABLE_TYPE(Subject);

 protected:
  Subject() = default;

  auto to_exception_ptr(Error* error) -> std::exception_ptr;

 public:
  ~Subject() override = default;

  auto GetType() const -> Class* override {
    return GetClass();
  }

  auto GetHashCode() const -> HashCode override {
    NOT_IMPLEMENTED(FATAL);  // TODO: implement
    return kInvalidHashCode;
  }

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
  static auto VisitClassPointerPointer(PointerPointerVisitor* vis) -> bool;
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
  TemplateSubject() = default;

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

  void Subscribe(const std::function<void(Object*)>& on_next,
                 const std::function<void(std::exception_ptr)>& on_error = {},
                 const std::function<void()>& on_completed = {}) override {
    get().get_observable().subscribe(on_next, on_error, on_completed);
  }
};

class PublishSubject : public TemplateSubject<rx::PublishSubject> {
 protected:
  PublishSubject() = default;

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
  ReplaySubject() = default;

 public:
  ~ReplaySubject() override = default;
  DECLARE_TYPE(ReplaySubject);

 public:
  static inline auto New() -> ReplaySubject* {
    return new ReplaySubject();
  }
};
}  // namespace gel

#endif  // GEL_SUBJECT_H
