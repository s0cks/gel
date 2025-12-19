add_subdirectory(gel/)

add_executable(gelle
  main.cc)
target_link_libraries(gelle
  PUBLIC gelrt)
if(CMAKE_BUILD_TYPE MATCHES "^[rR]elease")
  target_enable_iwyu(gelle)
endif()
