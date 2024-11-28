configure_file(scheme/scheme.h.in ${CMAKE_CURRENT_BINARY_DIR}/scheme/scheme.h)
file(GLOB_RECURSE SCM_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/scheme/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/scheme/*.cc"
  "${CMAKE_CURRENT_BINARY_DIR}/scheme/*.h")

find_package(Tracy CONFIG)
if(ENABLE_TRACING)
  if(NOT Tracy_FOUND)
    message(FATAL_ERROR "Tracy is required to enable tracing")
  endif()
  message(STATUS "enabling tracing w/ Tracy")
  list(APPEND SCM_LIBRARIES Tracy::TracyClient)
endif()

add_library(scheme STATIC
  ${SCM_SOURCES})
target_compile_options(scheme
  PUBLIC ${SCM_CXX_FLAGS} -fPIC)
target_include_directories(scheme
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
         ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(scheme
  PUBLIC ${SCM_LIBRARIES})

add_executable(scm
  scm.cc)
target_link_libraries(scm
  PUBLIC scheme)