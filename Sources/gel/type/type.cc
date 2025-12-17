#include "gel/type/type.h"

#include "gel/common.h"
#include "gel/type/bool.h"
#include "gel/type/nil.h"
#include "gel/type/number.h"
#include "gel/type/obj.h"
#include "gel/type/pair.h"
#include "gel/type/str.h"

namespace gel {
#ifdef GEL_DEBUG
void PrintAllTypeSizes() {
#define PRINT_TYPE_AND_SIZE(Name) LOG(INFO) << "sizeof(" << #Name << ") := " << sizeof(Name);
  FOR_EACH_TYPE(PRINT_TYPE_AND_SIZE);
#undef PRINT_TYPE_AND_SIZE
}
#endif  // GEL_DEBUG
}  // namespace gel
