/* csol
 * Copyright (c) 2017 Niels Sonnich Poulsen (http://nielssp.dk)
 * Licensed under the MIT license.
 * See the LICENSE file or http://opensource.org/licenses/MIT for more information.
 */

#include "error.h"

#include <stdarg.h>
#ifdef USE_PDCURSES
#include <curses.h>
#else
#include <ncurses.h>
#endif

void print_error(const char *format, ...) {
  va_list va;
  clear();
  wmove(stdscr, 0, 0);
  va_start(va, format);
  vw_printw(stdscr, format, va);
  printw("\nPress any key to continue");
  va_end(va);
  refresh();
  getch();
  clear();
}

