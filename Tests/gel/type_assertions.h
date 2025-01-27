#ifndef GEL_TYPE_ASSERTIONS_H
#define GEL_TYPE_ASSERTIONS_H

#include <gtest/gtest.h>

#include "gel/buffer.h"
#include "gel/error.h"
#include "gel/event_loop.h"
#include "gel/lambda.h"
#include "gel/macro.h"
#include "gel/module.h"
#include "gel/native_procedure.h"
#include "gel/object.h"
#include "gel/rx.h"
#include "gel/script.h"
#include "gel/symbol.h"

namespace gel::testing {
using namespace ::testing;

#define DEFINE_TYPE_ASSERTION(Name)                                                                         \
  static inline auto Is##Name(Name* rhs)->AssertionResult {                                                 \
    if (!rhs || !rhs->Is##Name())                                                                           \
      return AssertionFailure() << "expected " << (rhs ? rhs->ToString() : "null") << " to be a " << #Name; \
    return AssertionSuccess();                                                                              \
  }
FOR_EACH_TYPE(DEFINE_TYPE_ASSERTION)
#undef DEFINE_TYPE_ASSERTION

static inline auto IsNull(Object* d) -> AssertionResult {
  if (!d || !(d->IsPair() && d->AsPair()->IsEmpty()))
    return AssertionFailure() << "expected " << d << " to be Null.";
  return AssertionSuccess();
}

static inline auto IsBool(Object* rhs, const bool expected) -> AssertionResult {
  if (!rhs || !rhs->IsBool())
    return AssertionFailure() << "expected " << (rhs ? rhs->ToString() : "null") << " to be a Bool.";
  if (rhs->AsBool()->Get() != expected)
    return AssertionFailure() << "expected " << rhs->ToString() << " to be: " << (expected ? "true" : "false");
  return AssertionSuccess();
}

static inline auto IsTrue(Object* rhs) -> AssertionResult {
  return IsBool(rhs, true);
}

static inline auto IsFalse(Object* rhs) -> AssertionResult {
  return IsBool(rhs, false);
}

static inline auto IsSymbol(Object* rhs, const char* expected) -> AssertionResult {
  if (!rhs || !rhs->IsSymbol())
    return AssertionFailure() << "expected " << (rhs ? rhs->ToString() : "null") << " to be a Symbol.";
  if (rhs->AsSymbol()->GetFullyQualifiedName() != expected)
    return AssertionFailure() << "expected " << rhs->ToString() << " value to be: " << expected;
  return AssertionSuccess();
}
}  // namespace gel::testing

#endif  // GEL_TYPE_ASSERTIONS_H
