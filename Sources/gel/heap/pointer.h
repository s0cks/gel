#ifndef GEL_POINTER_H
#define GEL_POINTER_H

#include <functional>

#include "gel/common.h"
#include "gel/heap/tag.h"

namespace gel {
class Object;
class Pointer;
class PointerVisitor {
  DEFINE_NON_COPYABLE_TYPE(PointerVisitor);

 protected:
  PointerVisitor() = default;

 public:
  virtual ~PointerVisitor() = default;
  virtual auto Visit(Pointer* ptr) -> bool = 0;

  auto Visit(Object* ptr) -> bool;
};

class PointerVisitorWrapper : public PointerVisitor {
  using Callback = std::function<bool(Pointer*)>;
  DEFINE_NON_COPYABLE_TYPE(PointerVisitorWrapper);

 private:
  Callback delegate_;

 public:
  PointerVisitorWrapper(Callback delegate) :
    PointerVisitor(),
    delegate_(std::move(delegate)) {}
  ~PointerVisitorWrapper() override = default;

  auto Visit(Pointer* value) -> bool override {
    ASSERT(value);
    return delegate_(value);
  }
};

class PointerPointerVisitor {
  DEFINE_NON_COPYABLE_TYPE(PointerPointerVisitor);

 protected:
  PointerPointerVisitor() = default;

 public:
  virtual ~PointerPointerVisitor() = default;
  virtual auto Visit(Pointer** ptr) -> bool = 0;
};

class PointerPointerVisitorWrapper : public PointerPointerVisitor {
  using Callback = std::function<bool(Pointer**)>;
  DEFINE_NON_COPYABLE_TYPE(PointerPointerVisitorWrapper);

 private:
  Callback delegate_;

 public:
  PointerPointerVisitorWrapper(Callback delegate) :
    PointerPointerVisitor(),
    delegate_(delegate) {}
  ~PointerPointerVisitorWrapper() override = default;

  auto Visit(Pointer** value) -> bool override {
    return delegate_(value);
  }
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
  friend class Marker;
  friend class NewZone;
  friend class OldZone;
  friend class Collector;
  friend class PointerNotifier;
  DEFINE_NON_COPYABLE_TYPE(Pointer);

 public:
  using Predicate = std::function<bool(Pointer*)>;

  static inline auto TagEq(const Tag tag) -> Predicate {
    return [tag](Pointer* ptr) {
      return ptr && ((ptr->GetTag() & tag) == tag);
    };
  }

  static inline auto AnyTag() -> Predicate {
    return TagEq(Tag::Invalid());
  }

  static inline auto TagIsMarked() -> Predicate {
    return TagEq(Tag::Marked());
  }

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

 protected:
  auto VisitPointers(PointerVisitor* vis) -> bool;
  auto VisitPointerPointers(PointerPointerVisitor* vis) -> bool;

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

  auto IsRemembered() const -> bool {
    return GetTag().IsRemembered();
  }

  inline auto IsMarked() const -> bool {
    return GetTag().IsMarked();
  }

  inline auto IsFree() const -> bool {
    return GetTag().IsFree();
  }

  inline auto IsNew() const -> bool {
    return GetTag().IsNew();
  }

  inline auto IsOld() const -> bool {
    return GetTag().IsOld();
  }

  void SetMarked() {
    return tag_.SetMarkedBit();
  }

  void SetRemembered() {
    return tag_.SetRememberedBit();
  }

  auto Equals(Pointer* rhs) const -> bool {
    if (!rhs)
      return false;
    return GetStartingAddress() == rhs->GetStartingAddress() && GetTotalSize() == rhs->GetTotalSize();
  }

  auto GetTag() const -> const Tag& {
    return tag_;
  }

  friend auto operator<<(std::ostream& stream, const Pointer& rhs) -> std::ostream& {
    stream << "Pointer(";
    stream << "tag=" << rhs.GetTag() << ", ";
    stream << "starting_address=" << rhs.GetStartingAddressPointer() << ", ";
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

  static inline auto Old(const uword address, const uword size) -> Pointer* {
    return New(address, Tag::Old(size));
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

using PointerList = std::vector<Pointer*>;
}  // namespace gel

#endif  // GEL_POINTER_H
