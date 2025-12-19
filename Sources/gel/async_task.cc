#include "async_task.h"

#include "common.h"
#include "runtime.h"

namespace gel {
void Task::Execute() {
  ASSERT(callback_);
  GetRuntime()->Call(*callback_);
}
}  // namespace gel
