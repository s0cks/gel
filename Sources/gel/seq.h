#ifndef GEL_SEQ_H
#define GEL_SEQ_H

namespace gel {
class Seq : public Object {
  friend class Object;
  DEFINE_NON_COPYABLE_TYPE(Seq);

 private:
 protected:
  Seq() = default;

 public:
  ~Seq() override = default;
  virtual auto IsEmpty() const -> bool = 0;

  auto GetHashCode() const -> HashCode override;
  auto Equals(Object* rhs) const -> bool override;

  auto GetType() const -> Class* override {
    return GetClass();
  }

  auto AsSeq() -> Seq* override {
    return this;
  }

  static auto New(const ObjectList& args) -> Seq*;
  static auto operator new(const size_t sz) -> void*;
  static inline void operator delete(void* ptr) {
    ASSERT(ptr);
  }

 private:
  static Class* kClass;
  static void InitClass();
  static auto CreateClass() -> Class*;

 public:
  static auto VisitClassPointerPointer(PointerPointerVisitor* vis) -> bool;

  static inline auto GetClass() -> Class* {
    ASSERT(kClass);
    return kClass;
  }
};
}  // namespace gel

#endif  // GEL_SEQ_H
