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

add_library(test-plugin SHARED
  test_plugin.cc)
target_link_libraries(test-plugin PUBLIC gelle)

if(WIN32)

else()
  set(CURSES_NEED_NCURSES TRUE)
  find_package(Curses REQUIRED)
endif()

add_executable(test-repl
  test_repl.cc)

if(WIN32)

else()

target_include_directories(test-repl
  PRIVATE ${CURSES_INCLUDE_DIRS})
target_compile_options(test-repl
  PRIVATE ${CURSES_CFLAGS})
target_link_libraries(test-repl
  PRIVATE ncurses)

endif()

add_executable(gelrt
  main.cc)
target_link_libraries(gelrt
  PUBLIC gelle gflags::gflags)