configure_file(gel/gel.h.in ${CMAKE_CURRENT_BINARY_DIR}/gel/gel.h)
file(GLOB_RECURSE GEL_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/gel/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/gel/*.cc"
  "${CMAKE_CURRENT_BINARY_DIR}/gel/*.h")

list(APPEND GEL_LIBRARIES
  Threads::Threads
  glog::glog
  fmt::fmt
  units::units
  elfio::elfio
  FunctionalPlus::fplus
  $<IF:$<TARGET_EXISTS:libuv::uv_a>,libuv::uv_a,libuv::uv>
  graphviz::gvc graphviz::cgraph
  # OpenSSL::SSL OpenSSL::Crypto
  Backward::Backward
  glm::glm
  # BZip2::BZip2
  rapidjson)

find_path(TERMCOLOR_INCLUDE_DIRS "termcolor/termcolor.hpp")
if(WIN32)
  # TODO: Implement
else()
  add_library(gel-terminal INTERFACE)
  target_include_directories(gel-terminal
    INTERFACE # ${CURSES_INCLUDE_DIRS}
              ${TERMCOLOR_INCLUDE_DIRS})
  # target_compile_options(gel-terminal
  #   INTERFACE ${CURSES_CFLAGS})
  # target_link_libraries(gel-terminal
  #   INTERFACE ${CURSES_LIBRARIES} ncurses)
  add_library(gel::terminal ALIAS gel-terminal)

  list(APPEND GEL_LIBRARIES gel::terminal)
endif()

find_package(Tracy CONFIG)
if(ENABLE_TRACING)
  if(NOT Tracy_FOUND)
    message(FATAL_ERROR "Tracy is required to enable tracing")
  endif()
  message(STATUS "enabling tracing w/ Tracy")
  list(APPEND GEL_LIBRARIES
    Tracy::TracyClient)
endif()

add_library(gelle SHARED
  ${GEL_SOURCES})
target_compile_options(gelle
  PUBLIC ${GEL_CXX_FLAGS})
target_include_directories(gelle
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
         ${CMAKE_CURRENT_BINARY_DIR}
  PRIVATE ${TERMCOLOR_INCLUDE_DIRS})
target_link_libraries(gelle
  PUBLIC ${GEL_LIBRARIES} gflags::gflags)
if(CMAKE_BUILD_TYPE MATCHES "^[rR]elease")
  target_enable_iwyu(gelle)
endif()

list(APPEND GEL_BINDINGS
  env
  math
  os
  process
  fs
  rx
  test)

function(add_gel_binding target_name target_sources)
  add_library(${target_name} SHARED ${target_sources})
  target_link_libraries(${target_name} PUBLIC gelle)
  if(CMAKE_BUILD_TYPE MATCHES "^[rR]elease")
    target_enable_iwyu(${target_name})
  endif()
endfunction()

foreach(binding IN LISTS GEL_BINDINGS)
  add_gel_binding(${binding}-bindings
    bindings_${binding}.cc)
endforeach()

add_executable(gelrt
  main.cc)
target_link_libraries(gelrt
  PUBLIC gelle)
if(CMAKE_BUILD_TYPE MATCHES "^[rR]elease")
  target_enable_iwyu(gelrt)
endif()
