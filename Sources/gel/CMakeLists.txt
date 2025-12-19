configure_file(gel.h.in ${CMAKE_CURRENT_BINARY_DIR}/gel.h)
file(GLOB_RECURSE GEL_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
  "${CMAKE_CURRENT_BINARY_DIR}/*.h")

find_package(Threads REQUIRED)
find_package(glog CONFIG REQUIRED)

set(GFLAGS_USE_TARGET_NAMESPACE ON)
set(GFLAGS_BUILD_SHARED_LIBS ON)
find_package(gflags CONFIG REQUIRED)

find_package(fmt CONFIG REQUIRED)
find_package(units CONFIG REQUIRED)
find_package(libuv CONFIG REQUIRED)
find_package(RapidJSON CONFIG REQUIRED)
find_package(Backward CONFIG REQUIRED)
find_package(FunctionalPlus CONFIG REQUIRED)
find_package(elfio CONFIG REQUIRED)

include(Terminal)
list(APPEND GEL_LIBRARIES
  gflags::gflags
  gel::terminal
  Threads::Threads
  glog::glog
  fmt::fmt
  units::units
  elfio::elfio
  FunctionalPlus::fplus
  $<IF:$<TARGET_EXISTS:libuv::uv_a>,libuv::uv_a,libuv::uv>
  Backward::Backward
  rapidjson)

include(GraphViz)
if(GEL_ENABLE_GRAPHVIZ)
  LIST(APPEND GEL_LIBRARIES
    graphviz::gvc graphviz::cgraph)
endif()

add_library(gelrt SHARED
  ${GEL_SOURCES})
target_compile_options(gelrt
  PUBLIC ${GEL_CXX_FLAGS})
target_include_directories(gelrt
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
         ${CMAKE_CURRENT_BINARY_DIR}
  PRIVATE ${TERMCOLOR_INCLUDE_DIRS})
target_link_libraries(gelrt
  PUBLIC ${GEL_LIBRARIES})
if(CMAKE_BUILD_TYPE MATCHES "^[rR]elease")
  target_enable_iwyu(gelrt)
endif()
