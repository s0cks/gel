#include "type.h"

#include "common.h"
#include "types.h"

namespace gel {
#ifdef GEL_DEBUG
void PrintAllTypeSizes() {
#define PRINT_TYPE_AND_SIZE(Name) LOG(INFO) << "sizeof(" << #Name << ") := " << sizeof(Name);
  FOR_EACH_TYPE(PRINT_TYPE_AND_SIZE);
#undef PRINT_TYPE_AND_SIZE
}
#endif  // GEL_DEBUG
}  // namespace gel
