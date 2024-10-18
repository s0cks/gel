#ifndef SCM_TYPE_ASSERTIONS_H
#define SCM_TYPE_ASSERTIONS_H

#include <gtest/gtest.h>

#include "gtest/gtest.h"
#include "scheme/type.h"

namespace scm {
using namespace ::testing;

static inline auto IsNull(Datum* d) -> AssertionResult {
  if (!d || !d->IsNull())
    return AssertionFailure() << "expected " << d << " to be Null.";
  return AssertionSuccess();
}

}  // namespace scm

#endif  // SCM_TYPE_ASSERTIONS_H