#ifndef SCM_SECTION_H
#define SCM_SECTION_H

#include "scheme/common.h"
#include "scheme/platform.h"

namespace scm {
class Section {
  DEFINE_DEFAULT_COPYABLE_TYPE(Section);

 private:
  uword start_;
  uword size_;

 protected:
  Section() :
    start_(0),
    size_(0) {}
  Section(const uword start, const uword size) :
    start_(start),
    size_(size) {
    ASSERT(start >= 0);
    ASSERT(size == 0 || IsPow2(size));
  }

  virtual void Clear() {
    memset(GetStartingAddressPointer(), 0, GetSize());
  }

  virtual void SetRegion(const Section& rhs) {
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
  virtual ~Section() = default;

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

  inline auto IsAllocated() const -> bool {
    return GetStartingAddress() != UNALLOCATED && GetSize() >= 1;
  }

  auto operator==(const Section& rhs) const -> bool {
    return GetStartingAddress() == rhs.GetStartingAddress() && GetSize() == rhs.GetSize();
  }

  auto operator!=(const Section& rhs) const -> bool {
    return GetStartingAddress() != rhs.GetStartingAddress() || GetSize() != rhs.GetSize();
  }

  friend auto operator<<(std::ostream& stream, const Section& rhs) -> std::ostream& {
    stream << "Section(";
    stream << "start=" << rhs.GetStartingAddressPointer() << ", ";
    stream << "size=" << rhs.GetSize();
    stream << ")";
    return stream;
  }
};

class AllocationSection : public Section {
  DEFINE_DEFAULT_COPYABLE_TYPE(AllocationSection);

 protected:
  uword current_;

 protected:
  AllocationSection() :
    Section(),
    current_(0) {}
  AllocationSection(const uword start, const uword size) :
    Section(start, size),
    current_(start) {
    ASSERT(current_ >= 0);
    ASSERT(GetCurrentAddress() == GetStartingAddress());
  }

  void Clear() override {
    Section::Clear();
    current_ = GetStartingAddress();
  }

  void SetRegion(const Section& rhs) override {
    Section::SetRegion(rhs);
    current_ = rhs.GetStartingAddress();
  }

 public:
  ~AllocationSection() override = default;

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
}  // namespace scm

#endif  // SCM_SECTION_H
