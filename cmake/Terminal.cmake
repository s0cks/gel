find_path(TERMCOLOR_INCLUDE_DIRS "termcolor/termcolor.hpp")
if(WIN32)
  # TODO: Implement
else()
  set(CURSES_NEED_NCURSES TRUE)
  find_package(Curses REQUIRED)

  add_library(gel-term INTERFACE)
  target_include_directories(gel-term
    INTERFACE ${CURSES_INCLUDE_DIRS}
              ${TERMCOLOR_INCLUDE_DIRS})
  target_compile_options(gel-term
    INTERFACE ${CURSES_CFLAGS})
  target_link_libraries(gel-term
    INTERFACE ${CURSES_LIBRARIES}
              ncurses
              Threads::Threads
              glog::glog)
  add_library(gel::term ALIAS gel-term)
endif()
