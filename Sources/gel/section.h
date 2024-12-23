#ifndef GEL_SECTION_H
#define GEL_SECTION_H

#include "gel/common.h"
#include "gel/platform.h"

namespace gel {
class Region {
  DEFINE_DEFAULT_COPYABLE_TYPE(Region);

 private:
  uword start_;
  uword size_;

 protected:
  virtual void Clear() {
    memset(GetStartingAddressPointer(), 0, GetSize());
  }

  virtual void SetRegion(const Region& rhs) {
    operator=(rhs);
  }

  void SetSize(const uword size) {
    ASSERT(size >= 0);
    size_ = size;
  }

  void SetStartingAddress(const uword address) {
    ASSERT(address >= 0);
    start_ = address;
  }

 public:
  Region() :
    start_(0),
    size_(0) {}
  Region(const uword start, const uword size) :
    start_(start),
    size_(size) {
    ASSERT(start >= 0);
    // TODO: ASSERT(size == 0 || IsPow2(size));
  }
  virtual ~Region() = default;

  auto GetStartingAddress() const -> uword {
    return start_;
  }

  auto GetStartingAddressPointer() const -> void* {
    return (void*)GetStartingAddress();  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  auto GetSize() const -> uword {
    return size_;
  }

  auto GetEndingAddress() const -> uword {
    return GetStartingAddress() + GetSize();
  }

  auto GetEndingAddressPointer() const -> void* {
    return (void*)GetEndingAddress();  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  auto Contains(const uword address) const -> bool {
    return address >= GetStartingAddress() && address <= GetEndingAddress();
  }

  void CopyFrom(const uword start, const uword size) {
    ASSERT(start != UNALLOCATED);
    ASSERT(GetSize() >= size);
    memcpy(GetStartingAddressPointer(), (void*)start, size);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  inline auto IsAllocated() const -> bool {
    return GetStartingAddress() != UNALLOCATED && GetSize() >= 1;
  }

  auto operator==(const Region& rhs) const -> bool {
    return GetStartingAddress() == rhs.GetStartingAddress() && GetSize() == rhs.GetSize();
  }

  auto operator!=(const Region& rhs) const -> bool {
    return GetStartingAddress() != rhs.GetStartingAddress() || GetSize() != rhs.GetSize();
  }

  friend auto operator<<(std::ostream& stream, const Region& rhs) -> std::ostream& {
    stream << "Region(";
    stream << "start=" << rhs.GetStartingAddressPointer() << ", ";
    stream << "size=" << rhs.GetSize();
    stream << ")";
    return stream;
  }
};

class AllocationRegion : public Region {
  DEFINE_DEFAULT_COPYABLE_TYPE(AllocationRegion);

 protected:
  uword current_;

 protected:
  AllocationRegion() :
    Region(),
    current_(0) {}
  AllocationRegion(const uword start, const uword size) :
    Region(start, size),
    current_(start) {
    ASSERT(current_ >= 0);
    ASSERT(GetCurrentAddress() == GetStartingAddress());
  }

  void SetCurrent(const uword address) {
    ASSERT(address >= GetStartingAddress() && address <= GetEndingAddress());
    current_ = address;
  }

  void Clear() override {
    Region::Clear();
    current_ = GetStartingAddress();
  }

  void SetRegion(const Region& rhs) override {
    Region::SetRegion(rhs);
    current_ = rhs.GetStartingAddress();
  }

 public:
  ~AllocationRegion() override = default;

  auto GetCurrentAddress() const -> uword {
    return current_;
  }

  auto GetCurrentAddressPointer() const -> void* {
    return (void*)GetCurrentAddress();  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  auto IsEmpty() const -> bool {
    return GetCurrentAddress() == GetStartingAddress();
  }

  auto IsFull() const -> bool {
    return GetCurrentAddress() == GetEndingAddress();
  }

  virtual auto GetNumberOfBytesAllocated() const -> uword {
    return (GetCurrentAddress() - GetStartingAddress());
  }

  virtual auto GetAllocationPercent() const -> Percent {
    return Percent(GetNumberOfBytesAllocated(), GetSize());
  }

  virtual auto GetNumberOfBytesRemaining() const -> uword {
    return (GetSize() - GetNumberOfBytesAllocated());
  }

  virtual auto GetRemainingPercent() const -> Percent {
    return Percent(GetNumberOfBytesRemaining(), GetSize());
  }

  virtual auto TryAllocate(const uword size) -> uword = 0;
};
}  // namespace gel

#endif  // GEL_SECTION_H
