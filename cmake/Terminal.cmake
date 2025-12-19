#TODO(@s0cks): configure curses for repl?
find_path(TERMCOLOR_INCLUDE_DIRS "termcolor/termcolor.hpp")
if(WIN32)
  # TODO: Implement
else()
  add_library(gel-terminal INTERFACE)
  target_include_directories(gel-terminal
    INTERFACE # ${CURSES_INCLUDE_DIRS}
              ${TERMCOLOR_INCLUDE_DIRS})
  # target_compile_options(gel-terminal
  #   INTERFACE ${CURSES_CFLAGS})
  # target_link_libraries(gel-terminal
  #   INTERFACE ${CURSES_LIBRARIES} ncurses)
  add_library(gel::terminal ALIAS gel-terminal)
endif()
