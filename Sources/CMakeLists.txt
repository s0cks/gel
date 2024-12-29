configure_file(gel/gel.h.in ${CMAKE_CURRENT_BINARY_DIR}/gel/gel.h)
file(GLOB_RECURSE GEL_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/gel/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/gel/*.cc"
  "${CMAKE_CURRENT_BINARY_DIR}/gel/*.h")
list(REMOVE_ITEM GEL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/gel/plugin.h")

find_package(Tracy CONFIG)
if(ENABLE_TRACING)
  if(NOT Tracy_FOUND)
    message(FATAL_ERROR "Tracy is required to enable tracing")
  endif()
  message(STATUS "enabling tracing w/ Tracy")
  list(APPEND GEL_LIBRARIES Tracy::TracyClient)
endif()

add_library(gelle-api INTERFACE
  "${CMAKE_CURRENT_SOURCE_DIR}/gel/plugin.h")
target_include_directories(gelle-api
  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(gelle-api
  INTERFACE Threads::Threads glog::glog glog::glog fmt::fmt units::units $<IF:$<TARGET_EXISTS:libuv::uv_a>,libuv::uv_a,libuv::uv>)

add_library(gelle SHARED
  ${GEL_SOURCES})
target_compile_options(gelle
  PUBLIC ${GEL_CXX_FLAGS} -fPIC)
target_include_directories(gelle
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
         ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(gelle
  PUBLIC ${GEL_LIBRARIES} gelle-api gflags::gflags)

add_library(test-bindings SHARED
  bindings_test.cc)
target_link_libraries(test-bindings PUBLIC gelle)

add_library(env-bindings SHARED
  bindings_env.cc)
target_link_libraries(env-bindings PUBLIC gelle)

add_executable(gelrt
  main.cc)
target_link_libraries(gelrt
  PUBLIC gelle gflags::gflags)