enable_testing()

find_package(GTest CONFIG REQUIRED)

file(GLOB_RECURSE GEL_TEST_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/**/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/**/*.cc")

add_executable(gel-tests
  main.cc
  ${GEL_TEST_SOURCES})
target_include_directories(gel-tests
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
         ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(gel-tests
  PUBLIC gelle GTest::gtest GTest::gmock)

if(ENABLE_COVERAGE)
  create_coverage_target(gel-tests ${SCHEME_TEST_SOURCES})
endif()
