find_program(JQ NAMES "jq")
if(JQ)
  execute_process(
    COMMAND ${JQ} --version
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE JQ_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  string(REGEX MATCH "[0-9]+\.[0-9]+\.[0-9]+" JQ_VERSION "${JQ_VERSION}")

  message(STATUS "found jq v${JQ_VERSION}: ${JQ}")
  message(STATUS "jq options: ${JQ_OPTIONS}")

  function(set_launch_json_scripts launch_json_filename scripts)
    foreach(script ${${scripts}})
      list(APPEND launch_scripts "\"${script}\"")
    endforeach()
    message(STATUS "launch scripts: ${launch_scripts}")
    list(JOIN launch_scripts ", " LAUNCH_JSON_SCRIPTS)
    message(STATUS "launch scripts: ${LAUNCH_JSON_SCRIPTS}")
    execute_process(
      COMMAND "echo -E $(${JQ} '(.inputs[] | select(.id | contains(\"target-script\"))).options = [ ${LAUNCH_JSON_SCRIPTS} ]' ${launch_json_filename})"
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      COMMAND_ECHO STDOUT
      OUTPUT_VARIABLE UPDATE_RESULT
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    message(STATUS "result: ${UPDATE_RESULT}")
  endfunction()
endif()