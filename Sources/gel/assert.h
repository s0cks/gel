#ifndef GEL_ASSERT_H
#define GEL_ASSERT_H

#ifdef GEL_DEBUG

#include <cassert>
#define ASSERT(x) assert(x);

#else

#define ASSERT(x)

#endif  // GEL_DEBUG

#endif //GEL_ASSERT_H
