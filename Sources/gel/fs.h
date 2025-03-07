#ifndef GEL_FS_H
#define GEL_FS_H

#include "gel/common.h"
#include "gel/event_loop.h"

namespace gel {
class OpenFileRequest {
 private:
  uv_fs_t handle_{};
  std::string path_;

 public:
  explicit OpenFileRequest(std::string path) :
    path_(std::move(path)) {}
  ~OpenFileRequest() = default;

  auto handle() -> uv_fs_t* {
    return &handle_;
  }

  auto GetPath() const -> const std::string& {
    return path_;
  }

  DEFINE_NON_COPYABLE_TYPE(OpenFileRequest);
};
}  // namespace gel

#endif  // GEL_FS_H
