/* csol
 * Copyright (c) 2017 Niels Sonnich Poulsen (http://nielssp.dk)
 * Licensed under the MIT license.
 * See the LICENSE file or http://opensource.org/licenses/MIT for more information.
 */

#include "menu.h"

#include "theme.h"
#include "game.h"
#include "rc.h"

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#ifdef USE_PDCURSES
#include <curses.h>
#else
#include <ncurses.h>
#endif

Menu game_menu[] = {
  {NULL, NULL, 0, NULL}
};

Menu theme_menu[] = {
  {NULL, NULL, 0, NULL}
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

static void clear_box(int y, int x, int height, int width) {
  int i;
  move(y, x);
  while (height-- > 0) {
    move(y++, x);
    for (i = 0; i < width; i++) {
      addch(' ');
    }
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

static void ui_menu(int y, int x, Menu *menu, Menu **selection, int *y_max, int *x_max) {
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
  *y_max = y + 1;
  *x_max = x + max_length + 4;
}


static int compare_labels(const void *a, const void *b)
{
  return strcasecmp(((const Menu *) a)->label, ((const Menu *) b)->label);
}

void open_menu(int mnemonic, Menu *menu, Menu **menu_selection) {
  Menu *menu_item;
  for (menu_item = menu; menu_item->label; menu_item++) {
    char *l = menu_item->label;
    while (*l) {
      if (*l == '&') {
        if (tolower(l[1]) == mnemonic) {
          menu_selection[0] = menu_item;
          menu_selection[1] = menu_item->submenu;
        }
        break;
      }
      l++;
    }
    if (menu_selection[0]) {
      break;
    }
  }
}

static void close_menu(int y_min, int y_max, int x_min, int x_max, Menu **menu_selection) {
  menu_selection[0] = NULL;
  menu_selection[1] = NULL;
  clear_box(y_min, x_min, y_max - y_min, x_max - x_min);
  if (!show_menu) {
    move(0, 0);
    clrtoeol();
  }
}

int ui_menubar(Menu *menu, Menu **menu_selection) {
  int y_min = 1, y_max = 0, x_min = 0, x_max = 0;
  Menu *item;
  if (!show_menu && !menu_selection[0]) {
    return MENU_IS_CLOSED;
  }
  do {
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
            if (menu_selection[1]) {
              menu_selection[1] = item->submenu;
            }
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
            if (menu_selection[1]) {
              menu_selection[1] = item->submenu;
            }
          }
          ui_menu(1, x1 - 1, item->submenu, &menu_selection[1], &y_max, &x_max);
        }
        move(0, x2);
      } else {
        printw(" ");
        print_menu_label(item->label, !menu_selection[1]);
        printw(" ");
      }
    }
    if (menu_selection[0]) {
      Menu *menu_item;
      int ch = getch();
      switch (ch) {
        case KEY_LEFT:
          if (menu_selection[0] > menu) {
            menu_selection[0]--;
          } else {
            for (menu_item = menu; menu_item->label; menu_item++) {
              menu_selection[0] = menu_item;
            }
          }
          menu_selection[1] = menu_selection[0]->submenu;
          clear_box(y_min, x_min, y_max - y_min, x_max - x_min);
          return MENU_IS_OPEN;
        case KEY_RIGHT:
          menu_selection[0]++;
          if (!menu_selection[0]->label) {
            menu_selection[0] = menu;
            menu_selection[1] = menu->submenu;
          }
          menu_selection[1] = menu_selection[0]->submenu;
          clear_box(y_min, x_min, y_max - y_min, x_max - x_min);
          return MENU_IS_OPEN;
        case KEY_UP:
          if (menu_selection[1] && menu_selection[1] > menu_selection[0]->submenu) {
            menu_selection[1]--;
          } else if (menu_selection[0]->submenu) {
            for (menu_item = menu_selection[0]->submenu; menu_item->label; menu_item++) {
              menu_selection[1] = menu_item;
            }
          }
          break;
        case KEY_DOWN:
          if (menu_selection[1]) {
            menu_selection[1]++;
            if (!menu_selection[1]->label) {
              menu_selection[1] = menu_selection[0]->submenu;
            }
          } else {
            menu_selection[1] = menu_selection[0]->submenu;
          }
          break;
        case 10: /* enter */
        case 13: /* enter */
          if (menu_selection[1]) {
            int action = menu_selection[1]->action;
            close_menu(y_min, y_max, x_min, x_max, menu_selection);
            return action;
          } else {
            menu_selection[1] = menu_selection[0]->submenu;
          }
          break;
        case KEY_F(10):
        case 27:
          close_menu(y_min, y_max, x_min, x_max, menu_selection);
          return MENU_IS_OPEN;
        default:
          for (menu_item = menu_selection[1] ? menu_selection[0]->submenu : menu; menu_item->label; menu_item++) {
            char *l = menu_item->label;
            while (*l) {
              if (*l == '&') {
                if (tolower(l[1]) == ch) {
                  if (menu_selection[1]) {
                    int action = menu_item->action;
                    close_menu(y_min, y_max, x_min, x_max, menu_selection);
                    return action;
                  } else {
                    menu_selection[0] = menu_item;
                    menu_selection[1] = menu_item->submenu;
                    clear_box(y_min, x_min, y_max - y_min, x_max - x_min);
                    return MENU_IS_OPEN;
                  }
                }
                break;
              }
              l++;
            }
          }
          break;
      }
    }
  } while (menu_selection[0]);
  return MENU_IS_CLOSED;
}
