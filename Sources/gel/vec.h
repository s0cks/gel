#ifndef GEL_VEC_H
#define GEL_VEC_H

#include <utility>

#include "gel/common.h"
#ifdef GEL_ENABLE_GLM

#include <glm/glm.hpp>

#include "gel/number.h"
#include "gel/object.h"

namespace gel {
class Vec2 : public Object {
 private:
  glm::vec2 data_;

 protected:
  explicit Vec2(glm::vec2 data) :
    Object(),
    data_(std::move(data)) {}

 public:
  ~Vec2() override = default;

  auto Get() const -> const glm::vec2& {
    return data_;
  }

  auto GetX() const -> float {
    return data_[0];
  }

  auto GetY() const -> float {
    return data_[1];
  }

  DECLARE_TYPE(Vec2);

 public:
  static inline auto New(glm::vec2 data) -> Vec2* {
    return new Vec2(std::move(data));
  }

  static inline auto New(const double x, const double y) -> Vec2* {
    return New(glm::vec2(x, y));
  }

  static inline auto New(Number* x, Number* y) -> Vec2* {
    ASSERT(x);
    ASSERT(y);
    return New(x->GetDouble(), y->GetDouble());
  }

  static auto New(Pair* pair) -> Vec2*;
};

class Vec3 : public Object {
 private:
  glm::vec3 data_;

 protected:
  explicit Vec3(glm::vec3 data) :
    Object(),
    data_(std::move(data)) {}

 public:
  ~Vec3() override = default;

  auto Get() const -> const glm::vec3& {
    return data_;
  }

  auto GetX() const -> float {
    return data_[0];
  }

  auto GetY() const -> float {
    return data_[1];
  }

  auto GetZ() const -> float {
    return data_[2];
  }

  DECLARE_TYPE(Vec3);

 public:
  static inline auto New(glm::vec3 data) -> Vec3* {
    return new Vec3(std::move(data));
  }

  static inline auto New(const double x, const double y, const double z) -> Vec3* {
    return New(glm::vec3(x, y, z));
  }

  static inline auto New(Number* x, Number* y, Number* z) -> Vec3* {
    ASSERT(x);
    ASSERT(y);
    return New(x->GetDouble(), y->GetDouble(), z->GetDouble());
  }
};
}  // namespace gel

#endif  // GEL_ENABLE_GLM

#endif  // GEL_VEC_H
