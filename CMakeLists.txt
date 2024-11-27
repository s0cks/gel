cmake_minimum_required(VERSION 3.5)
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
find_package(gflags CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(units CONFIG REQUIRED)

list(APPEND SCM_LIBRARIES Threads::Threads glog::glog gflags::gflags fmt::fmt units::units)
if(ENABLE_RX)
  find_package(rxcpp CONFIG REQUIRED)
  list(APPEND SCM_LIBRARIES rxcpp)
  add_compile_definitions(SCM_ENABLE_RX)
endif()

if(CMAKE_BUILD_TYPE MATCHES "^[Dd]ebug")
  add_compile_definitions(SCM_DEBUG)
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