#include <ncurses.h>

#include <cstdlib>
#include <string>
#include <vector>

auto main(int argc, char** argv) -> int {
  initscr();
  cbreak();
  noecho();

  int y, x;
  getmaxyx(stdscr, y, x);

  WINDOW* input_win = newwin(3, x - 2, y - 5, 1);
  WINDOW* output_win = newwin(y - 8, x - 2, 1, 1);
  box(input_win, 0, 0);
  box(output_win, 0, 0);

  refresh();
  wrefresh(input_win);
  wrefresh(output_win);

  std::vector<std::string> history{};

  std::string input_line;
  while (true) {
    wmove(input_win, 1, 1);
    wclrtoeol(input_win);
    wprintw(input_win, "> ");
    wrefresh(input_win);

    int ch = -1;
    while ((ch = wgetch(input_win)) != '\n') {
      if (ch == 127) {  // Backspace
        if (!input_line.empty()) {
          input_line.pop_back();
          wmove(input_win, 1, input_line.size() + 2);
          wdelch(input_win);
        }
      } else if (ch == KEY_UP) {
        if (history.empty())
          continue;
        input_line = history.back();
        history.pop_back();
      } else {
        input_line += static_cast<char>(ch);
        waddch(input_win, ch);
      }
    }

    if (input_line == "exit") {
      break;
    }

    history.push_back(input_line);
    // Process input (e.g., evaluate expression)
    std::string output = "Result: " + input_line;

    // Append output to output window
    wmove(output_win, 1, 1);
    wclrtoeol(output_win);
    wprintw(output_win, "%s", output.c_str());
    wrefresh(output_win);

    input_line.clear();
  }

  endwin();
  return EXIT_SUCCESS;
}