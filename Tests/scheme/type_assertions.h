#ifndef SCM_TYPE_ASSERTIONS_H
#define SCM_TYPE_ASSERTIONS_H

#include <gtest/gtest.h>

#include "gtest/gtest.h"
#include "scheme/error.h"
#include "scheme/expression_compiled.h"
#include "scheme/lambda.h"
#include "scheme/macro.h"
#include "scheme/native_procedure.h"
#include "scheme/type.h"

namespace scm {
using namespace ::testing;

#define DEFINE_TYPE_ASSERTION(Name)                                                                         \
  static inline auto Is##Name(Name* rhs) -> AssertionResult {                                               \
    if (!rhs || !rhs->Is##Name())                                                                           \
      return AssertionFailure() << "expected " << (rhs ? rhs->ToString() : "null") << " to be a " << #Name; \
    return AssertionSuccess();                                                                              \
  }
FOR_EACH_TYPE(DEFINE_TYPE_ASSERTION)
#undef DEFINE_TYPE_ASSERTION

static inline auto IsNull(Datum* d) -> AssertionResult {
  if (!d || !d->IsNull())
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
}  // namespace scm

#endif  // SCM_TYPE_ASSERTIONS_H