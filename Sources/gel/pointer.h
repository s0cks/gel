#ifndef GEL_POINTER_H
#define GEL_POINTER_H

#include "gel/common.h"
#include "gel/platform.h"
#include "gel/tag.h"

namespace gel {
class Pointer;
class PointerVisitor {
  DEFINE_NON_COPYABLE_TYPE(PointerVisitor);

 protected:
  PointerVisitor() = default;

 public:
  virtual ~PointerVisitor() = default;
  virtual auto Visit(Pointer* ptr) -> bool = 0;
};

class PointerPointerVisitor {
  DEFINE_NON_COPYABLE_TYPE(PointerPointerVisitor);

 protected:
  PointerPointerVisitor() = default;

 public:
  virtual ~PointerPointerVisitor() = default;
  virtual auto Visit(Pointer** ptr) -> bool = 0;
};

class PointerIterator {
  DEFINE_NON_COPYABLE_TYPE(PointerIterator);

 protected:
  PointerIterator() = default;

 public:
  virtual ~PointerIterator() = default;
  virtual auto HasNext() const -> bool = 0;
  virtual auto Next() -> Pointer* = 0;
};

class Object;
class Pointer {
  friend class NewZone;
  friend class OldZone;
  friend class Collector;
  DEFINE_NON_COPYABLE_TYPE(Pointer);

 private:
  Tag tag_;
  uword forwarding_ = UNALLOCATED;

  explicit Pointer(const Tag& tag) :
    tag_(tag) {}

  void SetForwardingAddress(const uword address) {
    forwarding_ = address;
  }

  void SetTag(const Tag& rhs) {
    tag_ = rhs;
  }

  inline void ClearTag() {
    return SetTag(kInvalidTag);
  }

  inline auto tag() -> Tag& {
    return tag_;
  }

 public:
  ~Pointer() = default;

  auto GetStartingAddress() const -> uword {
    return (uword)this;  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  auto GetStartingAddressPointer() const -> void* {
    return (void*)GetStartingAddress();  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  inline auto GetObjectSize() const -> uword {
    return GetTag().GetSize();
  }

  auto GetObjectAddress() const -> uword {
    return GetStartingAddress() + sizeof(Pointer);
  }

  auto GetObjectAddressPointer() const -> void* {
    return (void*)GetObjectAddress();  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  inline auto GetObjectPointer() const -> Object* {
    return ((Object*)GetObjectAddressPointer());  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  template <class T>
  inline auto As() const -> T* {
    return (T*)GetObjectPointer();
  }

  auto GetTotalSize() const -> uword {
    return sizeof(Pointer) + GetObjectSize();
  }

  auto GetEndingAddress() const -> uword {
    return GetStartingAddress() + GetTotalSize();
  }

  auto GetEndingAddressPointer() const -> void* {
    return (void*)GetEndingAddress();  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  auto GetForwardingAddress() const -> uword {
    return forwarding_;
  }

  auto GetForwardingAddressPointer() const -> void* {
    return (void*)GetForwardingAddress();  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  inline auto IsForwarding() const -> bool {
    return GetForwardingAddress() != UNALLOCATED;
  }

  auto GetTag() const -> const Tag& {
    return tag_;
  }

  friend auto operator<<(std::ostream& stream, const Pointer& rhs) -> std::ostream& {
    stream << "Pointer(";
    stream << "tag=" << rhs.GetTag() << ", ";
    stream << "starting_address=" << rhs.GetStartingAddress() << ", ";
    stream << "forwarding_address=" << rhs.GetForwardingAddressPointer();
    stream << ")";
    return stream;
  }

 private:
  static inline auto New(const uword address, const Tag& tag) -> Pointer* {
    return new ((void*)address) Pointer(tag);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  static inline auto New(const uword address, const uword size) -> Pointer* {
    return New(address, Tag::New(size));
  }

  static inline auto Copy(const uword address, const Pointer* ptr) -> Pointer* {
    const auto new_ptr = New(address, ptr->GetTag());
    ASSERT(new_ptr);
    memcpy(new_ptr->GetObjectAddressPointer(), ptr->GetObjectAddressPointer(), ptr->GetObjectSize());
    return new_ptr;
  }

 public:
  static inline auto At(const uword address) -> Pointer* {
    return (Pointer*)address;  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }
};
}  // namespace gel

#endif  // GEL_POINTER_H
