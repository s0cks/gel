#include "gel/async_task.h"

#include "gel/common.h"
#include "gel/runtime.h"

namespace gel {
void Task::Execute() {
  ASSERT(callback_);
  GetRuntime()->Call(callback_);
}
}  // namespace gel