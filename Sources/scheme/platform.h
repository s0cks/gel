#ifndef SCM_PLATFORM_H
#define SCM_PLATFORM_H

#include <cstdint>

#if defined(_M_X64) || defined(__x86_64__)
#define ARCH_IS_X64 1
#elif defined(_M_IX86) || defined(__i386__)
#define ARCH_IS_X32 1
#elif defined(__ARMEL__)
#define ARCH_IS_ARM 1
#elif defined(__aarch64__)
#define ARCH_IS_ARM64 1
#endif

#if defined(__linux__) || defined(__FreeBSD__)
#define OS_IS_LINUX 1
#elif defined(__APPLE__)
#define OS_IS_OSX 1
#elif defined(_WIN32)
#define OS_IS_WINDOWS 1
#endif

namespace scm {  // TODO: cleanup this file
static constexpr int kInt8SizeLog2 = 0;
static constexpr int kInt8Size = 1 << kInt8SizeLog2;
static constexpr int kInt16SizeLog2 = 1;
static constexpr int kInt16Size = 1 << kInt16SizeLog2;
static constexpr int kInt32SizeLog2 = 2;
static constexpr int kInt32Size = 1 << kInt32SizeLog2;
static constexpr int kInt64SizeLog2 = 3;
static constexpr int kInt64Size = 1 << kInt64SizeLog2;

static constexpr int kBitsPerByteLog2 = 3;
static constexpr int kBitsPerByte = 1 << kBitsPerByteLog2;
static constexpr int kBitsPerInt8 = kInt8Size * kBitsPerByte;
static constexpr int kBitsPerInt16 = kInt16Size * kBitsPerByte;
static constexpr int kBitsPerInt32 = kInt32Size * kBitsPerByte;
static constexpr int kBitsPerInt64 = kInt64Size * kBitsPerByte;

using byte = int8_t;
using ubyte = uint8_t;
using word = intptr_t;
using uword = uintptr_t;

#define UNALLOCATED 0

#ifdef ARCH_IS_X32
static constexpr int kWordSizeLog2 = kInt32SizeLog2;
#elif defined(ARCH_IS_ARM64) || defined(ARCH_IS_X64)
static constexpr int kWordSizeLog2 = kInt64SizeLog2;
#endif
// cppcheck-suppress truncLongCastAssignment
static constexpr uint64_t kWordSize = 1 << kWordSizeLog2;

static constexpr int kBitsPerWordLog2 = kWordSizeLog2 + kBitsPerByteLog2;
static constexpr int kBitsPerWord = 1 << kBitsPerWordLog2;

static constexpr uword kUWordOne = 1U;
}  // namespace scm

#endif  // SCM_PLATFORM_H
