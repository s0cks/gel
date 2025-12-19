add_subdirectory(gel/)

add_executable(gelrt
  main.cc)
target_link_libraries(gelrt
  PUBLIC gelle)
if(CMAKE_BUILD_TYPE MATCHES "^[rR]elease")
  target_enable_iwyu(gelrt)
endif()
