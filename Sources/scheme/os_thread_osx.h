#ifndef SCM_OS_THREAD_OSX_H
#define SCM_OS_THREAD_OSX_H

#ifndef SCM_OS_THREAD_H
#error "Please #include <scheme/os_thread.h> instead of <scheme/os_thread_osx.h>"
#endif  // SCM_OS_THREAD_H

#include <pthread.h>

namespace scm {
static const int kThreadNameMaxLength = 16;
static const int kThreadMaxResultLength = 128;

using ThreadLocalKey = pthread_key_t;
using ThreadId = pthread_t;
using ThreadHandler = void (*)(void*);

#ifndef PTHREAD_OK
#define PTHREAD_OK 0
#endif //PTHREAD_OK

}  // namespace scm

#endif  // SCM_OS_THREAD_OSX_H
