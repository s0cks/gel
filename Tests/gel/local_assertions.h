#ifndef GEL_LOCAL_ASSERTIONS_H
#define GEL_LOCAL_ASSERTIONS_H

#include <gtest/gtest.h>

#include "gel/local.h"
#include "gel/object.h"

namespace gel {
using namespace ::testing;

static inline auto HasNoValue(LocalVariable* rhs) -> AssertionResult {
  ASSERT(rhs);
  if (rhs->HasValue())
    return AssertionFailure() << "expected " << (*rhs) << " to not have a value.";
  return AssertionSuccess();
}

static inline auto HasValue(LocalVariable* rhs, Datum* expected) -> AssertionResult {
  ASSERT(rhs);
  ASSERT(expected);
  if (!rhs->HasValue())
    return AssertionFailure() << "expected " << (*rhs) << " to have a value.";
  if (!rhs->GetValue()->Equals(expected))
    return AssertionFailure() << "expected " << (*rhs) << " to have value: " << expected->ToString();
  return AssertionSuccess();
}

static inline auto HasOwner(LocalVariable* rhs, LocalScope* expected_owner) -> AssertionResult {
  ASSERT(rhs);
  ASSERT(expected_owner);
  if (!rhs->HasOwner())
    return AssertionFailure() << "expected " << (*rhs) << " to have an owner.";
  if (rhs->GetOwner() != expected_owner)
    return AssertionFailure() << "expected " << (*rhs) << " to have owner: " << expected_owner;
  return AssertionSuccess();
}

static inline auto IsLocal(LocalVariable* local, const char* expected_name, const uint64_t expected_index) -> AssertionResult {
  ASSERT(local);
  ASSERT(expected_name);
  ASSERT(expected_index >= 0);
  if (local->GetName() != expected_name)
    return AssertionFailure() << "expected " << (*local) << " to have name: " << expected_name;
  if (local->GetIndex() != expected_index)
    return AssertionFailure() << "expected " << (*local) << " to have index: " << expected_index;
  return AssertionSuccess();
}
}  // namespace gel

#endif  // GEL_LOCAL_ASSERTIONS_H
