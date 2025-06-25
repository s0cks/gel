find_program(IWYU "include-what-you-use")
if(IWYU)
  message(STATUS "found iwyu: ${IWYU}")
  set(IWYU_OPTS "-Xiwyu" "--mapping_file=${CMAKE_SOURCE_DIR}/iwyu.imp")

  macro(target_enable_iwyu target_name)
    set_property(TARGET ${target_name} PROPERTY CXX_INCLUDE_WHAT_YOU_USE "${IWYU};;${IWYU_OPTS}")
  endmacro()
endif()