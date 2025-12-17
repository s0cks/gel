if(ENABLE_TRACING)
  find_package(Tracy CONFIG REQUIRED)

  add_library(gel-tracing INTERFACE)
  target_link_libraries(gel-tracing
    INTERFACE Tracy::TracyClient)
  target_compile_definitions(gel-tracing
    INTERFACE GEL_ENABLE_TRACING)
endif()
