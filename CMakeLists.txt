cmake_minimum_required(VERSION 3.29.3)
message(STATUS "path: $ENV{PATH}")

include(BuildInfo)
project(
  ${BUILD_JSON_NAME}
  VERSION ${BUILD_JSON_VERSION}
  HOMEPAGE_URL ${BUILD_JSON_HOMEPAGE}
  LANGUAGES C CXX)

include(Options)
# add_compile_options(
#     -Wall
#     -fms-extensions
#     -Wno-deprecated
#     -Wno-exceptions
#     -Wno-overloaded-virtual
#     -Wno-sign-compare
#     -Wno-writable-strings # TODO: remove no-writable-strings
#     -Wno-sizeof-pointer-memaccess
#     -fPIC
#     -frtti)

find_package(Threads REQUIRED)
find_package(glog CONFIG REQUIRED)
set(GFLAGS_USE_TARGET_NAMESPACE ON)
set(GFLAGS_BUILD_SHARED_LIBS ON)
find_package(gflags CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(units CONFIG REQUIRED)
find_package(libuv CONFIG REQUIRED)

list(APPEND GEL_LIBRARIES)

if(CMAKE_BUILD_TYPE MATCHES "^[Dd]ebug")
  add_compile_definitions(GEL_DEBUG)
endif()

add_subdirectory(Sources)

option(ENABLE_TESTS "Enable tests." ON)
if(ENABLE_TESTS)
  add_subdirectory(Tests)
endif()

option(ENABLE_BENCHMARKS "Enable benchmarks." ON)
if(ENABLE_BENCHMARKS)
  add_subdirectory(Benchmarks)
endif()