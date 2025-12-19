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
include(Tracing)

if(CMAKE_BUILD_TYPE MATCHES "^[Dd]ebug")
  add_compile_definitions(GEL_DEBUG)
endif()

add_subdirectory(Sources)
add_subdirectory(scripts)

option(ENABLE_TESTS "Enable tests." ON)
if(ENABLE_TESTS)
  add_subdirectory(Tests)
endif()

option(ENABLE_BENCHMARKS "Enable benchmarks." ON)
if(ENABLE_BENCHMARKS)
  add_subdirectory(Benchmarks)
endif()

option(GEL_BUILD_BINDINGS "Build bindings" ON)
if(GEL_BUILD_BINDINGS)
  add_subdirectory(Bindings/)
endif()
