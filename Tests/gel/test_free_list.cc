#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "gel/common.h"
#include "gel/free_list.h"
#include "gel/memory_region.h"
#include "gmock/gmock.h"

namespace gel {
class FreeListTest : public ::testing::Test {
  static constexpr const auto kTestRegionSize = 128 * 1024 * 1024;
  DEFINE_NON_COPYABLE_TYPE(FreeListTest);

 private:
  MemoryRegion region_;
  FreeList free_list_;

 protected:
  FreeListTest() :
    ::testing::Test(),
    region_(kTestRegionSize, MemoryRegion::kReadWrite),
    free_list_(region_) {}

  inline auto region() const -> const MemoryRegion& {
    return region_;
  }

  inline auto region() -> MemoryRegion& {
    return region_;
  }

  inline auto free_list() const -> const FreeList& {
    return free_list_;
  }

  auto free_list() -> FreeList& {
    return free_list_;
  }

 protected:
  static inline auto NewFreePointer(const uword size) -> FreePointer* {
    const auto total_size = sizeof(FreePointer) + size;
    const auto data = malloc(total_size);  // NOLINT(cppcoreguidelines-no-malloc)
    ASSERT(data);
    return FreePointer::New(data, size);
  }

  static inline void DeleteFreePointer(FreePointer* free_ptr) {
    ASSERT(free_ptr);
    free(free_ptr);  // NOLINT(cppcoreguidelines-no-malloc)
  }

 public:
  ~FreeListTest() override = default;

  void SetUp() override {
    ASSERT_TRUE(free_list_.IsEmpty());
  }

  void TearDown() override {
    free_list_.Clear();
    ASSERT_TRUE(free_list_.IsEmpty());
  }
};

class MockFreePointerVisitor : public FreePointerVisitor {
  DEFINE_NON_COPYABLE_TYPE(MockFreePointerVisitor);

 public:
  MockFreePointerVisitor() = default;
  ~MockFreePointerVisitor() override = default;
  MOCK_METHOD(bool, Visit, (FreePointer*));  // NOLINT
};

TEST_F(FreeListTest, Test_Remove_Fails_NotInFreeListRegion) {
  ASSERT_NO_FATAL_FAILURE(free_list().Remove(Region(0, 19882)));
}

TEST_F(FreeListTest, Test_Remove_Fails_NotInFreeList) {
  ASSERT_NO_FATAL_FAILURE(free_list().Remove(Region(free_list().head()->GetStartingAddress(), 100)));
}

TEST_F(FreeListTest, Test_Remove_RemovesTheHeadNode) {
  ASSERT_NO_FATAL_FAILURE(free_list().Remove(free_list().head()->region()));
  ASSERT_THAT(free_list().head(), ::testing::IsNull());
}

TEST_F(FreeListTest, Test_Insert_Fails_NotInFreeListRegion) {
  const auto free_ptr = NewFreePointer(128);
  ASSERT_FALSE(free_list().Insert(free_ptr));
  DeleteFreePointer(free_ptr);
}

TEST_F(FreeListTest, Test_Insert_Fails_DuplicateItem) {
  ASSERT_FALSE(free_list().Insert(free_list().head()));
}

TEST_F(FreeListTest, Test_Insert_IsSuccessful) {
  const auto head = free_list().head();
  ASSERT(head);
  ASSERT_NO_FATAL_FAILURE(free_list().Remove(head->region()));
  ASSERT_TRUE(free_list().Insert(head));
}

MATCHER_P(FreePointerEquals, rhs, "") {  // NOLINT
  return arg && arg->Equals(rhs);
}

TEST_F(FreeListTest, Test_VisitFreePointers) {
  MockFreePointerVisitor vis;
  EXPECT_CALL(vis, Visit(FreePointerEquals(free_list().head()))).WillOnce(::testing::Return(true));
  ASSERT_TRUE(free_list().VisitFreePointers(&vis));
}

TEST_F(FreeListTest, Test_TryAllocate) {  // NOLINT
  DLOG(INFO) << "free list before:";
  PrintFreeList(free_list());

  const auto new_address = free_list().TryAllocate(sizeof(uword));
  ASSERT_NE(new_address, UNALLOCATED);
  DLOG(INFO) << "free_ptr: " << FreePointer::At(new_address - sizeof(FreePointer))->ToString();

  DLOG(INFO) << "free list after:";
  PrintFreeList(free_list());
}
}  // namespace gel