enable_testing()

find_package(GTest CONFIG REQUIRED)

file(GLOB_RECURSE SCHEME_TEST_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/**/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/**/*.cc")

add_executable(scheme-tests
  main.cc
  ${SCHEME_TEST_SOURCES})
target_include_directories(scheme-tests
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
         ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(scheme-tests
  PUBLIC scheme GTest::gtest GTest::gmock)

if(ENABLE_COVERAGE)
  create_coverage_target(scheme-tests ${SCHEME_TEST_SOURCES})
endif()
