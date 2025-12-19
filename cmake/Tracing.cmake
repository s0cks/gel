option(GEL_ENABLE_TRACING "Compile w/ tracing enabled" OFF)
if(GEL_ENABLE_TRACING)
  find_package(Tracy CONFIG)
  if(NOT Tracy_FOUND)
    message(FATAL_ERROR "Tracy is required to enable tracing")
  endif()
  message(STATUS "enabling tracing w/ Tracy")

  add_library(gel-tracing INTERFACE)
  target_link_libraries(gel-tracing
    INTERFACE Tracy::TracyClient)

  add_library(gel::tracing ALIAS gel-tracing)
endif()

