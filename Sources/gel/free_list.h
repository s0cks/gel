#ifndef GEL_FREE_LIST_H
#define GEL_FREE_LIST_H

#include "gel/common.h"
#include "gel/platform.h"
#include "gel/pointer.h"
#include "gel/section.h"

namespace gel {
class FreePointer {
  friend class FreeList;
  DEFINE_NON_COPYABLE_TYPE(FreePointer);

 private:
  Tag tag_;
  uword next_ = UNALLOCATED;

 protected:
  FreePointer(const Tag& tag) :
    tag_(tag) {}

  void SetNext(FreePointer* ptr) {
    next_ = ptr ? ptr->GetStartingAddress() : UNALLOCATED;
  }

 public:
  ~FreePointer() = default;

  auto tag() -> Tag& {
    return tag_;
  }

  auto tag() const -> const Tag& {
    return tag_;
  }

  auto GetStartingAddress() const -> uword {
    return (uword)this;  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  auto GetStartingAddressPointer() const -> void* {
    return (void*)GetStartingAddress();  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  auto GetEndingAddress() const -> uword {
    return GetStartingAddress() + GetTotalSize();
  }

  auto GetEndingAddressPointer() const -> void* {
    return (void*)GetEndingAddress();  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  inline auto GetPointerSize() const -> uword {
    return tag().GetSize();
  }

  inline auto GetTotalSize() const -> uword {
    return sizeof(Pointer) + GetPointerSize();
  }

  auto GetNext() const -> FreePointer* {
    return (FreePointer*)next_;  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  inline auto HasNext() const -> bool {
    return GetNext() != nullptr;
  }

  auto Equals(FreePointer* rhs) const -> bool;
  auto ToString() const -> std::string;
  friend auto operator<<(std::ostream& stream, const FreePointer& rhs) -> std::ostream&;

 private:
  static inline auto New(const uword address, const Tag& tag) -> FreePointer* {
    ASSERT(address > UNALLOCATED);
    return new ((void*)address) FreePointer(tag);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

 public:
  static auto At(const uword address) -> FreePointer* {
    return (FreePointer*)address;  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }
};

class FreeList : public Section {
  friend class OldZone;
  DEFINE_DEFAULT_COPYABLE_TYPE(FreeList);

 private:
  FreePointer* head_;

 protected:
  FreeList() :
    Section(),
    head_(nullptr) {}
  FreeList(const uword start_address, const uword size) :
    Section(start_address, size),
    head_(FreePointer::New(start_address, Tag::Old(size))) {
    ASSERT(head_);
  }

 public:
  ~FreeList() override = default;

  auto TryAllocate(const uword size) -> uword;
  auto VisitFreePointers(const std::function<bool(FreePointer*)>& vis) const -> bool;

  friend auto operator<<(std::ostream& stream, const FreeList& rhs) -> std::ostream& {
    stream << "FreeList(";
    stream << "starting_address=" << rhs.GetStartingAddressPointer() << ", ";
    stream << "total_size=" << units::data::byte_t(static_cast<double>(rhs.GetSize())) << ", ";
    if (rhs.head_)
      stream << "head=" << rhs.head_->ToString();
    stream << ")";
    return stream;
  }
};

#ifdef GEL_DEBUG

static inline auto PrintFreePointers(const FreeList& free_list) -> bool {
  static const auto kPrintFreePointer = [](FreePointer* ptr) {
    ASSERT(ptr);
    LOG(INFO) << " - " << ptr->ToString();
    return true;
  };
  return free_list.VisitFreePointers(kPrintFreePointer);
}

#endif  // GEL_DEBUG
}  // namespace gel

#endif  // GEL_FREE_LIST_H
