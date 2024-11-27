#ifndef SCM_RX_H
#define SCM_RX_H

#ifdef SCM_ENABLE_RX

#include <rxcpp/rx.hpp>

#include "scheme/object.h"

namespace scm {
namespace rx {
using rxcpp::subjects::subject;
using namespace rxcpp;
using error_ptr = rxcpp::util::error_ptr;
using rxcpp::util::what;
}  // namespace rx

class Subscription : public Datum {
 private:
  Subscription() = default;

 public:
  ~Subscription() override = default;

  DECLARE_TYPE(Subscription);

 public:
  static inline auto New() -> Subscription* {
    return new Subscription();
  }
};

class Observable : public Datum {
 private:
  Observable() = default;

 public:
  ~Observable() override = default;

  DECLARE_TYPE(Observable);

 public:
  static inline auto New() -> Observable* {
    return new Observable();
  }
};
}  // namespace scm

#endif  // SCM_ENABLE_RX
#endif  // SCM_RX_H
