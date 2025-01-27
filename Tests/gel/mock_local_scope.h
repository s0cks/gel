#ifndef GEL_MOCK_LOCAL_SCOPE_H
#define GEL_MOCK_LOCAL_SCOPE_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "gel/local_scope.h"

namespace gel {
class MockLocalScope : public LocalScope {
  DEFINE_NON_COPYABLE_TYPE(MockLocalScope);

 public:
  MockLocalScope() = default;
  ~MockLocalScope() override = default;
  MOCK_METHOD(LocalScope*, GetParent, (), (const, override));                                                     // NOLINT
  MOCK_METHOD(bool, Has, (Symbol * symbol, const bool recursive), (const, override));                             // NOLINT
  MOCK_METHOD(LocalVariable*, Add, (LocalVariable * local), (override));                                          // NOLINT
  MOCK_METHOD(void, AddAll, (LocalScope * scope), (override));                                                    // NOLINT
  MOCK_METHOD(bool, Lookup, (Symbol * symbol, LocalVariable** result, const bool recursive), (const, override));  // NOLINT
  MOCK_METHOD(bool, IsEmpty, (), (const, override));                                                              // NOLINT
  MOCK_METHOD(uint64_t, GetNumberOfLocals, (), (const, override));                                                // NOLINT
};
}  // namespace gel

#endif  // GEL_MOCK_LOCAL_SCOPE_H
