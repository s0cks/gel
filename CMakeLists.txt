cmake_minimum_required(VERSION 3.29.3)
message(STATUS "path: $ENV{PATH}")

include(BuildInfo)
project(
  ${BUILD_JSON_NAME}
  VERSION ${BUILD_JSON_VERSION}
  HOMEPAGE_URL ${BUILD_JSON_HOMEPAGE}
  LANGUAGES C CXX)

include(IWYU)
include(Options)
find_package(Threads REQUIRED)
find_package(glog CONFIG REQUIRED)

set(GFLAGS_USE_TARGET_NAMESPACE ON)
set(GFLAGS_BUILD_SHARED_LIBS ON)
find_package(gflags CONFIG REQUIRED)

set(OPENSSL_USE_STATIC_LIBS FALSE)
find_package(OpenSSL REQUIRED)

if(GEL_ENABLE_GLM)
  add_compile_definitions(GEL_ENABLE_GLM)
  find_package(glm CONFIG REQUIRED)
endif()

find_package(Graphviz REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(units CONFIG REQUIRED)
find_package(libuv CONFIG REQUIRED)
find_package(RapidJSON CONFIG REQUIRED)
find_package(Backward CONFIG REQUIRED)
find_package(Bzip2 REQUIRED)
find_package(FunctionalPlus CONFIG REQUIRED)
find_package(elfio CONFIG REQUIRED)
find_package(RPP CONFIG REQUIRED)

if(CMAKE_BUILD_TYPE MATCHES "^[Dd]ebug")
  add_compile_definitions(GEL_DEBUG)
endif()

add_subdirectory(vm/)
add_subdirectory(scripts/)

option(ENABLE_TESTS "Enable tests." ON)
if(ENABLE_TESTS)
  add_subdirectory(Tests/)
endif()

option(ENABLE_BENCHMARKS "Enable benchmarks." ON)
if(ENABLE_BENCHMARKS)
  add_subdirectory(Benchmarks/)
endif()
