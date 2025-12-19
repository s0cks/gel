function(add_gel_binding target_name target_sources)
  add_library(${target_name}-bindings SHARED ${target_sources})
  target_link_libraries(${target_name}-bindings PUBLIC gelrt)
  if(CMAKE_BUILD_TYPE MATCHES "^[rR]elease")
    target_enable_iwyu(${target_name}-bindings)
  endif()
endfunction()

option(GEL_ENABLE_BINDINGS_MATH "Enable math bindings" ON)
if(GEL_ENABLE_BINDINGS_MATH)
  add_subdirectory(math/)
endif()

option(GEL_ENABLE_BINDINGS_OS "Enable os bindings" ON)
if(GEL_ENABLE_BINDINGS_OS)
  add_subdirectory(os/)
endif()

option(GEL_ENABLE_BINDINGS_FS "Enable fs bindings" ON)
if(GEL_ENABLE_BINDINGS_FS)
  add_subdirectory(fs/)
endif()

list(APPEND GEL_BINDINGS
  env
  process
  rx
  test)

foreach(binding IN LISTS GEL_BINDINGS)
  add_gel_binding(${binding}
    bindings_${binding}.cc)
endforeach()

