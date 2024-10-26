cmake_minimum_required(VERSION 3.29.3})
include(BuildInfo)
project(
  ${BUILD_JSON_NAME}
  VERSION ${BUILD_JSON_VERSION}
  HOMEPAGE_URL ${BUILD_JSON_HOMEPAGE}
  LANGUAGES C CXX)

list(APPEND SCM_CXX_FLAGS
     -Wall
     -Wno-sign-compare
     -Wno-writable-strings # TODO: remove no-writable-strings
     -frtti)

option(WERROR "Turn warnings into errors." ON)
if(WERROR)
  list(APPEND SCM_CXX_FLAGS -Werror)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "([aA]pple)?[cC]lang|GNU")
  option(ENABLE_ADDRESS_SCANITIZER "Enable Clang AddressSanitizer" OFF)
  if(ENABLE_ADDRESS_SCANITIZER AND CMAKE_BUILD_TYPE MATCHES "^[Dd]ebug")
    list(APPEND SCM_CXX_FLAGS -01 -fno-omit-frame-pointer -fsanitize=address)
  endif()

  option(ENABLE_UNDEFINED_SANITIZER "Enable Clang UndefinedBehaviorSanitizer"
         OFF)
  if(ENABLE_UNDEFINED_SANITIZER AND CMAKE_BUILD_TYPE MATCHES "^[Dd]ebug")
    list(APPEND SCM_CXX_FLAGS -fsanitize=undefined -fsanitize=integer)
  endif()

  option(ENABLE_CLANG_TIDY "Enable clang-tidy" ON)
  if(ENABLE_CLANG_TIDY)
    include(ClangTidy)
    enable_clang_tidy()
  endif()
endif()

option(ENABLE_DOXYGEN "Build documentation using Doxygen" ON)
if(ENABLE_DOXYGEN)
  include(Doxygen)
endif()

option(ENABLE_COVERAGE "Enable code coverage reports using gcov/lcov" OFF)
if(ENABLE_COVERAGE)
  include(Coverage)
  # TODO: cleanup this logic, get it into Coverage.cmake
  if(${CMAKE_CXX_COMPILER_ID} MATCHES "([aA]pple)?[cC]lang|GNU")
    list(
      APPEND
      SCM_CXX_FLAGS
      -O0
      -g
      -fprofile-instr-generate
      -fcoverage-mapping
      --coverage)
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # TODO: implement
  else()
    message(FATAL_ERROR "unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
  endif()
endif()

find_package(Threads REQUIRED)
find_package(glog CONFIG REQUIRED)
find_package(gflags CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

if(APPLE)
  link_directories(/opt/homebrew/lib/)
endif()
link_libraries(Threads::Threads glog::glog gflags::gflags fmt::fmt)

if(CMAKE_BUILD_TYPE MATCHES "^[Dd]ebug")
  add_compile_definitions(SCM_DEBUG)
endif()

option(ENABLE_TRACING "Enable Tracy tracing." OFF)
if(ENABLE_TRACING)
  add_compile_definitions(SCM_TRACING)
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
