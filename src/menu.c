/* csol
 * Copyright (c) 2017 Niels Sonnich Poulsen (http://nielssp.dk)
 * Licensed under the MIT license.
 * See the LICENSE file or http://opensource.org/licenses/MIT for more information.
 */

#include "menu.h"

#include "theme.h"
#include "game.h"

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#ifdef USE_PDCURSES
#include <curses.h>
#else
#include <ncurses.h>
#endif

Menu game_menu[] = {
  {NULL, NULL, NULL}
};

Menu theme_menu[] = {
  {NULL, NULL, NULL}
};

void ui_message(const char *format, ...) {
  va_list va;
  move(0, 0);
  clrtoeol();
  va_start(va, format);
  move(0, 0);
  vw_printw(stdscr, format, va);
  va_end(va);
}

int ui_confirm(const char *message) {
  ui_message("%s (Y/N)", message);
  switch (getch()) {
    case 'y': case 'Y':
      return 1;
    default:
      return 0;
  }
}

void ui_box(int y, int x, int height, int width, int fill) {
  int i;
  move(y, x);
  for (i = 0; i < width; i++) {
    if (height && i == 0) {
      addch(ACS_ULCORNER);
    } else if (height && i + 1 >= width) {
      addch(ACS_URCORNER);
    } else {
      addch(ACS_HLINE);
    }
  }
  if (height > 1) {
    while (--height > 1) {
      move(++y, x);
      addch(ACS_VLINE);
      if (fill) {
        for (i = 2; i < width; i++) {
          addch(' ');
        }
      } else {
        move(y, x + width - 1);
      }
      addch(ACS_VLINE);
    }
    move(++y, x);
    for (i = 0; i < width; i++) {
      if (i == 0) {
        addch(ACS_LLCORNER);
      } else if (i + 1 >= width) {
        addch(ACS_LRCORNER);
      } else {
        addch(ACS_HLINE);
      }
    }
  }
}

static void print_menu_label(const char *label, int show_mnemonic) {
  while (*label) {
    if (*label == '&') {
      label++;
      if (show_mnemonic) {
        attron(A_BOLD);
      }
      if (*label) {
        printw("%c", *label);
        label++;
      }
      if (show_mnemonic) {
        attroff(A_BOLD);
      }
    } else {
      printw("%c", *label);
      label++;
    }
  }
}

static int ui_menu(int y, int x, Menu *menu, Menu **selection) {
  Menu *item;
  int max_length = 0;
  int height = 0;
  for (item = menu; item->label; item++) {
    int length = strlen(item->label);
    if (item->key) {
      length += 2 + strlen(item->key);
    }
    if (length > max_length) {
      max_length = length;
    }
    height++;
  }
  ui_box(y++, x, height + 2, max_length + 4, 1);
  for (item = menu; item->label; item++) {
    if (item == *selection) {
      attron(A_REVERSE);
    }
    mvprintw(y, x + 1, " %*s ", max_length, item->key ? item-> key : "");
    move(y, x + 2);
    print_menu_label(item->label, *selection != NULL);
    if (item == *selection) {
      attroff(A_REVERSE);
    }
    y++;
  }
  return y;
}


static int compare_labels(const void *a, const void *b)
{
  return strcasecmp(((const Menu *) a)->label, ((const Menu *) b)->label);
}

int ui_menubar(Menu *menu, Menu **menu_selection) {
  int y, y_max = 0;
  Menu *item;
  move(0, 0);
  printw(" ");
  for (item = menu; item->label; item++) {
    if (item == menu_selection[0]) {
      int x1 = getcurx(stdscr), x2;
      attron(A_REVERSE);
      printw(" ");
      print_menu_label(item->label, !menu_selection[1]);
      printw(" ");
      attroff(A_REVERSE);
      x2 = getcurx(stdscr);
      if (item->submenu) {
        if (item->submenu == game_menu) {
          GameList *list;
          int size = 0, i = 0;
          ui_box(1, x1 - 1, 3, 14, 1);
          mvprintw(2, x1 + 1, "Loading...");
          refresh();
          load_game_dirs();
          for (list = list_games(); list; list = list->next) {
            size++;
          }
          item->submenu = malloc(sizeof(Menu) * (size + 1));
          for (list = list_games(); list; list = list->next) {
            item->submenu[i].label = list->game->title;
            item->submenu[i].key = NULL;
            item->submenu[i].submenu = NULL;
            i++;
          }
          item->submenu[i].label = NULL;
          qsort(item->submenu, size, sizeof(Menu), compare_labels);
        } else if (item->submenu == theme_menu) {
          ThemeList *list;
          int size = 0, i = 0;
          ui_box(1, x1 - 1, 3, 14, 1);
          mvprintw(2, x1 + 1, "Loading...");
          refresh();
          load_theme_dirs();
          for (list = list_themes(); list; list = list->next) {
            size++;
          }
          item->submenu = malloc(sizeof(Menu) * (size + 1));
          for (list = list_themes(); list; list = list->next) {
            item->submenu[i].label = list->theme->name;
            item->submenu[i].key = NULL;
            item->submenu[i].submenu = NULL;
            i++;
          }
          item->submenu[i].label = NULL;
          qsort(item->submenu, size, sizeof(Menu), compare_labels);
        }
        y = ui_menu(1, x1 - 1, item->submenu, &menu_selection[1]);
        if (y > y_max) {
          y_max = y;
        }
      }
      move(0, x2);
    } else {
      printw(" ");
      print_menu_label(item->label, !menu_selection[1]);
      printw(" ");
    }
  }
  return y_max;
}
