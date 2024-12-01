#ifndef GEL_TYPE_ASSERTIONS_H
#define GEL_TYPE_ASSERTIONS_H

#include <gtest/gtest.h>

#include "gel/error.h"
#include "gel/lambda.h"
#include "gel/macro.h"
#include "gel/native_procedure.h"
#include "gel/object.h"
#include "gel/rx.h"
#include "gel/script.h"
#include "gtest/gtest.h"

namespace gel {
using namespace ::testing;

#define DEFINE_TYPE_ASSERTION(Name)                                                                         \
  static inline auto Is##Name(Name* rhs)->AssertionResult {                                                 \
    if (!rhs || !rhs->Is##Name())                                                                           \
      return AssertionFailure() << "expected " << (rhs ? rhs->ToString() : "null") << " to be a " << #Name; \
    return AssertionSuccess();                                                                              \
  }
FOR_EACH_TYPE(DEFINE_TYPE_ASSERTION)
#undef DEFINE_TYPE_ASSERTION

static inline auto IsNull(Datum* d) -> AssertionResult {
  if (!d || !(d->IsPair() && d->AsPair()->IsEmpty()))
    return AssertionFailure() << "expected " << d << " to be Null.";
  return AssertionSuccess();
}

static inline auto IsBool(Datum* rhs, const bool expected) -> AssertionResult {
  if (!rhs || !rhs->IsBool())
    return AssertionFailure() << "expected " << (rhs ? rhs->ToString() : "null") << " to be a Bool.";
  if (rhs->AsBool()->Get() != expected)
    return AssertionFailure() << "expected " << rhs->ToString() << " to be: " << (expected ? "true" : "false");
  return AssertionSuccess();
}

static inline auto IsTrue(Datum* rhs) -> AssertionResult {
  return IsBool(rhs, true);
}

static inline auto IsFalse(Datum* rhs) -> AssertionResult {
  return IsBool(rhs, false);
}

static inline auto IsSymbol(Datum* rhs, const char* expected) -> AssertionResult {
  if (!rhs || !rhs->IsSymbol())
    return AssertionFailure() << "expected " << (rhs ? rhs->ToString() : "null") << " to be a Symbol.";
  if (rhs->AsSymbol()->Get() != expected)
    return AssertionFailure() << "expected " << rhs->ToString() << " value to be: " << expected;
  return AssertionSuccess();
}
}  // namespace gel

#endif  // GEL_TYPE_ASSERTIONS_H
