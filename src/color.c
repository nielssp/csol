/* csol
 * Copyright (c) 2017 Niels Sonnich Poulsen (http://nielssp.dk)
 * Licensed under the MIT license.
 * See the LICENSE file or http://opensource.org/licenses/MIT for more information.
 */

#include "color.h"

#include <string.h>
#ifdef USE_PDCURSES
#include <curses.h>
#else
#include <ncurses.h>
#endif

struct color_name {
  char *name;
  int index;
};

struct color_name default_colors[] = {
  {"default", -1},
  {"black", COLOR_BLACK},
  {"red", COLOR_RED},
  {"green", COLOR_GREEN},
  {"yellow", COLOR_YELLOW},
  {"blue", COLOR_BLUE},
  {"magenta", COLOR_MAGENTA},
  {"cyan", COLOR_CYAN},
  {"white", COLOR_WHITE},
  {"bright_black", COLOR_BLACK | 8},
  {"bright_red", COLOR_RED | 8},
  {"bright_green", COLOR_GREEN | 8},
  {"bright_yellow", COLOR_YELLOW | 8},
  {"bright_blue", COLOR_BLUE | 8},
  {"bright_magenta", COLOR_MAGENTA | 8},
  {"bright_cyan", COLOR_CYAN | 8},
  {"bright_white", COLOR_WHITE | 8},
  {NULL, -1}
};

static short find_color(Theme *theme, short index, char *name) {
  if (name) {
    struct color_name *default_color;
    Color *color;
    for (color = theme->colors; color; color = color->next) {
      if (strcmp(color->name, name) == 0) {
        return color->index;
      }
    }
    for (default_color = default_colors; default_color->name; default_color++) {
      if (strcmp(default_color->name, name) == 0) {
        return default_color->index;
      }
    }
  }
  return index;
}

static void find_and_init_color_pair(Theme *theme, short index, ColorPair color_pair) {
  short fg = find_color(theme, color_pair.fg, color_pair.fg_name);
  short bg = find_color(theme, color_pair.bg, color_pair.bg_name);
  if (fg >= COLORS) {
    printf("Unsupported color index: %d\n", fg);
  }
  if (bg >= COLORS) {
    printf("Unsupported color index: %d\n", bg);
  }
  init_pair(index, fg, bg);
}

void init_theme_colors(Theme *theme) {
  Color *color;
  short index = 15;
  start_color();
  for (color = theme->colors; color; color = color->next) {
    short color_index = color->name ? ++index : color->index;
    color_content(color_index, &color->old_red, &color->old_green, &color->old_blue);
    if (init_color(color_index, color->red, color->green, color->blue) == 0) {
      color->index = color_index;
    } else {
      printf("Unable to initialize color: %s\n", color->name);
    }
  }
  find_and_init_color_pair(theme, COLOR_PAIR_BACKGROUND, theme->background);
  find_and_init_color_pair(theme, COLOR_PAIR_EMPTY, theme->empty_layout.color);
  find_and_init_color_pair(theme, COLOR_PAIR_BACK, theme->back_layout.color);
  find_and_init_color_pair(theme, COLOR_PAIR_RED, theme->red_layout.color);
  find_and_init_color_pair(theme, COLOR_PAIR_BLACK, theme->black_layout.color);
}

void ui_list_colors() {
  int i, pair = 0;
  initscr();
  curs_set(0);
  start_color();
  printw("Colors: %d  Color pairs: %d\n", COLORS, COLOR_PAIRS);
  printw("Standard colors\n");
  for (i = 0; i < 8; i++) {
    switch (i) {
      case COLOR_BLACK:
        printw("%-8s", "black");
        break;
      case COLOR_RED:
        printw("%-8s", "red");
        break;
      case COLOR_GREEN:
        printw("%-8s", "green");
        break;
      case COLOR_YELLOW:
        printw("%-8s", "yellow");
        break;
      case COLOR_BLUE:
        printw("%-8s", "blue");
        break;
      case COLOR_MAGENTA:
        printw("%-8s", "magenta");
        break;
      case COLOR_CYAN:
        printw("%-8s", "cyan");
        break;
      case COLOR_WHITE:
        printw("%-8s", "white");
        break;
      default:
        printw("%-8s", "");
    }
  }
  printw("\n");
  for (i = 0; i < 8; i++) {
    init_pair(++pair, i, 0);
    attron(COLOR_PAIR(pair));
    printw(" %2d ", i);
    init_pair(++pair, 0, i);
    attron(COLOR_PAIR(pair));
    printw(" %2d ", i);
    attroff(COLOR_PAIR(pair));
  }
  if (COLORS > 8) {
    printw("\nHigh-intensity colors\n");
    for (i = 8; i < 16; i++) {
      init_pair(++pair, i, 0);
      attron(COLOR_PAIR(pair));
      printw(" %2d ", i);
      init_pair(++pair, 0, i);
      attron(COLOR_PAIR(pair));
      printw(" %2d ", i);
      attroff(COLOR_PAIR(pair));
    }
    if (COLORS > 16) {
      printw("\n216 colors\n");
      for (i = 0; i < 6; i++) {
        int j;
        for (j = 0; j < 18; j++) {
          int bg = 16 + i * 36 + j;
          init_pair(++pair, 7, bg);
          attron(COLOR_PAIR(pair));
          printw(" %3d", bg);
          attroff(COLOR_PAIR(pair));
        }
        printw("\n");
      }
      for (i = 0; i < 6; i++) {
        int j;
        for (j = 18; j < 36; j++) {
          int bg = 16 + i * 36 + j;
          init_pair(++pair, 0, bg);
          attron(COLOR_PAIR(pair));
          printw(" %3d", bg);
          attroff(COLOR_PAIR(pair));
        }
        printw("\n");
      }
      printw("Grayscale colors\n");
      for (i = 0; i < 24; i++) {
        int bg = 232 + i;
        int fg = i >= 12 ? 0 : 7;
        init_pair(++pair, fg, bg);
        attron(COLOR_PAIR(pair));
        printw("%3d", bg);
        attroff(COLOR_PAIR(pair));
      }
    }
  }
  getch();
  endwin();
  use_default_colors();
}
