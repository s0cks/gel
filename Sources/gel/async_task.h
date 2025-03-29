#ifndef GEL_ASYNC_TASK_H
#define GEL_ASYNC_TASK_H

#include "gel/object.h"

namespace gel {
class Task {
  DEFINE_DEFAULT_COPYABLE_TYPE(Task);

 private:
  Procedure* callback_;

 public:
  explicit Task(Procedure* callback = nullptr) :
    callback_(callback) {}
  ~Task() = default;

  void Execute();
};
}  // namespace gel

#endif  // GEL_ASYNC_TASK_H
