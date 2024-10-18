file(GLOB_RECURSE SCHEME_BENCHMARK_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/scheme/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/scheme/*.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/scheme/**/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/scheme/**/*.cc")

find_package(benchmark CONFIG REQUIRED)

add_executable(scheme-benchmarks
  main.cc
  ${SCHEME_BENCHMARK_SOURCES})
target_compile_options(scheme-benchmarks
  PUBLIC ${SCHEME_COMPILE_OPTS})
target_include_directories(scheme-benchmarks
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(scheme-benchmarks
  PUBLIC scheme benchmark::benchmark)