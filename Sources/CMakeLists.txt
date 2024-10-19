configure_file(scheme/scheme.h.in ${CMAKE_CURRENT_BINARY_DIR}/scheme/scheme.h)
file(GLOB_RECURSE SCHEME_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/**/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/**/*.cc"
  "${CMAKE_CURRENT_BINARY_DIR}/**/*.h")

find_package(Graphviz REQUIRED)
find_package(Tracy CONFIG)

list(APPEND SCM_LIBRARIES
  graphviz::gvc
  graphviz::cgraph)

if(ENABLE_TRACING)
  if(NOT Tracy_FOUND)
    message(FATAL_ERROR "Tracy is required to enable tracing")
  endif()
  message(STATUS "enabling tracing w/ Tracy")
  list(APPEND SCM_LIBRARIES Tracy::TracyClient)
endif()

add_library(scheme STATIC
  ${SCHEME_SOURCES})
target_compile_options(scheme
  PUBLIC ${SCHEME_COMPILE_OPTS})
target_include_directories(scheme
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
         ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(scheme
  PUBLIC ${SCM_LIBRARIES})