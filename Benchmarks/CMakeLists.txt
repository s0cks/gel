file(
  GLOB_RECURSE
  GEL_BENCHMARK_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/gel/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/gel/*.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/gel/**/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/gel/**/*.cc")

find_package(benchmark CONFIG REQUIRED)

add_executable(gel-benchmarks main.cc ${GEL_BENCHMARK_SOURCES})
target_compile_options(gel-benchmarks PUBLIC ${GEL_COMPILE_OPTS})
target_include_directories(gel-benchmarks PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(gel-benchmarks PUBLIC gelle benchmark::benchmark gflags::gflags)
