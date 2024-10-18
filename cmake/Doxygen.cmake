# TODO: move docs output to ${CMAKE_SOURCE_DIR}/docs/
include(FetchContent)
function(download_file url hash output_dir)
  FetchContent_Declare(
    download_${hash}
    URL ${url}
    URL_HASH SHA256=${hash}
    SOURCE_DIR ${output_dir}
    DOWNLOAD_NO_EXTRACT ON)
  if(NOT download_${hash}_POPULATED)
    FetchContent_Populate(download_${hash})
  endif()
endfunction()

# doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
  set(DOXYGEN_BUILD_DIR ${CMAKE_SOURCE_DIR}/docs)

  option(ENABLE_DOXYGEN_AWESOME "Enable doxygen-awesome-css" ON)
  if(ENABLE_DOXYGEN_AWESOME)
    set(DOXYGEN_AWESOME_REPO "jothepro/doxygen-awesome-css")
    set(DOXYGEN_AWESOME_VERSION "2.3.3")
    set(DOXYGEN_AWESOME_HASH
        96555949c7b03e132fd3fc7a9b3193c84ae690492a65de7039608a1015c2f848)
    set(DOXYGEN_AWESOME_DIR "${CMAKE_BINARY_DIR}/doxygen-awesome-css-${DOXYGEN_AWESOME_VERSION}")
    download_file(
      https://github.com/${DOXYGEN_AWESOME_REPO}/archive/refs/tags/v${DOXYGEN_AWESOME_VERSION}.zip
      ${DOXYGEN_AWESOME_HASH}
      ${CMAKE_BINARY_DIR})
    file(ARCHIVE_EXTRACT INPUT
      ${CMAKE_BINARY_DIR}/v${DOXYGEN_AWESOME_VERSION}.zip
      DESTINATION ${CMAKE_BINARY_DIR})
    list(APPEND DOXYGEN_AWESOME_STYLESHEETS
      ${DOXYGEN_AWESOME_DIR}/doxygen-awesome.css)
    list(APPEND DOXYGEN_EXTRA_STYLESHEETS
      ${DOXYGEN_AWESOME_STYLESHEETS})

    list(APPEND DOXYGEN_AWESOME_FILES
      ${DOXYGEN_AWESOME_DIR}/doxygen-awesome-darkmode-toggle.js)
    list(APPEND DOXYGEN_EXTRA_FILES
      ${DOXYGEN_AWESOME_FILES})
  endif()

  set(DOXYGEN_IN ${CMAKE_SOURCE_DIR}/Doxyfile.in)
  set(DOXYGEN_OUT ${CMAKE_BINARY_DIR}/Doxyfile)
  set(DOXYGEN_INPUT_DIR ${CMAKE_SOURCE_DIR}/Sources/)

  list(APPEND DOXYGEN_EXCLUDES "*/vcpkg_installed/*")
  list(JOIN DOXYGEN_EXCLUDES " " DOXYGEN_EXCLUDE_PATTERNS)

  include(ProcessorCount)
  ProcessorCount(NUM_PROCESSORS)

  list(JOIN DOXYGEN_EXTRA_FILES " " DOXYGEN_HTML_EXTRA_FILES)
  list(JOIN DOXYGEN_EXTRA_STYLESHEETS " " DOXYGEN_HTML_EXTRA_STYLESHEETS)
  configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

  add_custom_target(
    doxygen
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
    WORKING_DIRECTORY ${DOXYGEN_BUILD_DIR}
    COMMENT "Generate API documentation with Doxygen"
    VERBATIM)
endif()
