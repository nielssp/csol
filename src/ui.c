/* csol
 * Copyright (c) 2017 Niels Sonnich Poulsen (http://nielssp.dk)
 * Licensed under the MIT license.
 * See the LICENSE file or http://opensource.org/licenses/MIT for more information.
 */

#include <stdlib.h>
#if defined(MSDOS) || defined(_WIN32)
#define MSDOS
#include <curses.h>
#else
#include <ncurses.h>
#endif
#include <locale.h>
#include <time.h>
#include <ctype.h>
#include <string.h>

#include "ui.h"
#include "rc.h"
#include "card.h"
#include "theme.h"

#define COLOR_PAIR_BACKGROUND 1
#define COLOR_PAIR_EMPTY 2
#define COLOR_PAIR_BACK 3
#define COLOR_PAIR_RED 4
#define COLOR_PAIR_BLACK 5

int deals = 0;

int cur_x = 0;
int cur_y = 0;

int max_x = 0;
int max_y = 0;

int win_w = 0;
int win_h = 0;

int off_y = 0;

Card *selection = NULL;
Pile *selection_pile = NULL;
Card *cursor_card = NULL;
Pile *cursor_pile = NULL;

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

void print_card_name_l(int y, int x, Card *card, Theme *theme) {
  if (y < 0 || y >= win_h) {
    return;
  }
  switch (card->rank) {
    case ACE:
      mvprintw(y, x, "A");
      break;
    case KING:
      mvprintw(y, x, "K");
      break;
    case QUEEN:
      mvprintw(y, x, "Q");
      break;
    case JACK:
      mvprintw(y, x, "J");
      break;
    default:
      mvprintw(y, x, "%d", card->rank);
  }
#ifdef MSDOS
  raw_output(1);
#endif
  printw(card_suit(card, theme));
#ifdef MSDOS
  raw_output(0);
#endif
}

void print_card_name_r(int y, int x, Card *card, Theme *theme) {
  if (y < 0 || y >= win_h) {
    return;
  }
#ifdef MSDOS
  raw_output(1);
#endif
  mvprintw(y, x - 1 - (card->rank == 10), card_suit(card, theme));
#ifdef MSDOS
  raw_output(0);
#endif
  switch (card->rank) {
    case ACE:
      printw("A");
      break;
    case KING:
      printw("K");
      break;
    case QUEEN:
      printw("Q");
      break;
    case JACK:
      printw("J");
      break;
    default:
      printw("%d", card->rank);
  }
}

void print_layout(int y, int x, Card *card, Layout layout, int full, Theme *theme) {
  if (y >= win_h) {
    return;
  }
  if (y >= 0) {
    mvprintw(y, x, layout.top);
  }
  if (full && theme->height > 1) {
    int i;
    for (i = 1; i < theme->height - 1; i++) {
      if (y + i >= 0 && y + i < win_h) {
        mvprintw(y + i, x, layout.middle);
      }
    }
    if (y + theme->height > 0 && y + theme->height <= win_h) {
      mvprintw(y + theme->height - 1, x, layout.bottom);
    }
  }
}

void print_card(int y, int x, Card *card, int full, Theme *theme) {
  if (card == selection) {
    attron(A_REVERSE);
  }
  if (card->suit & BOTTOM) {
    attron(COLOR_PAIR(COLOR_PAIR_EMPTY));
    print_layout(y, x, card, theme->empty_layout, full, theme);
    if (card->rank > 0) {
      print_card_name_l(y, x + theme->empty_layout.left_padding, card, theme);
    }
  } else if (!card->up) {
    attron(COLOR_PAIR(COLOR_PAIR_BACK));
    print_layout(y, x, card, theme->back_layout, full, theme);
  } else {
    int left_padding, right_padding;
    if (card->suit & RED) {
      attron(COLOR_PAIR(COLOR_PAIR_RED));
      print_layout(y, x, card, theme->red_layout, full, theme);
      left_padding = theme->red_layout.left_padding;
      right_padding = theme->red_layout.right_padding;
    } else {
      attron(COLOR_PAIR(COLOR_PAIR_BLACK));
      print_layout(y, x, card, theme->black_layout, full, theme);
      left_padding = theme->black_layout.left_padding;
      right_padding = theme->black_layout.right_padding;
    }
    if (full && theme->height > 1) {
      print_card_name_r(y + theme->height - 1, x + theme->width - (right_padding + 1), card, theme);
    }
    print_card_name_l(y, x + left_padding, card, theme);
  }
  attroff(A_REVERSE);
}

int theme_y(int y, Theme *theme) {
  return theme->y_margin + off_y + y;
}

int theme_x(int x, Theme *theme) {
  return theme->x_margin + x * (theme->width + theme->x_spacing);
}

int print_card_in_grid(int y, int x, Card *card, int full, Theme *theme) {
  int y2 = y + full * (theme->height - 1);
  if (y2 > max_y) max_y = y2;
  if (x > max_x) max_x = x;
  if (y <= cur_y && y2 >= cur_y && x == cur_x) cursor_card = card;
  card->x = x;
  card->y = y;
  y = theme_y(y, theme);
  x = theme_x(x, theme);
  if (win_h - 1 < y) {
    return 0;
  }
  print_card(y, x, card, full, theme);
  return cursor_card == card;
}

int print_card_top(int y, int x, Card *card, Theme *theme) {
  return print_card_in_grid(y, x, card, 0, theme);
}

int print_card_full(int y, int x, Card *card, Theme *theme) {
  return print_card_in_grid(y, x, card, 1, theme);
}

int print_stack(int y, int x, Card *bottom, Theme *theme) {
  if (bottom->next) {
    return print_stack(y, x, bottom->next, theme);
  } else {
    return print_card_full(y, x, bottom, theme);
  }
}

int print_tableau(int y, int x, Card *bottom, Theme *theme) {
  if (bottom->next && bottom->suit & BOTTOM) {
    return print_tableau(y, x, bottom->next, theme);
  } else {
    if (bottom->next) {
      int cursor_below = print_card_top(y, x, bottom, theme);
      return print_tableau(y + 1, x, bottom->next, theme) || cursor_below;
    } else {
      int cursor_below = cur_x == x && cur_y >= y;
      return print_card_full(y, x, bottom, theme) || cursor_below;
    }
  }
}

void print_pile(Pile *pile, Theme *theme) {
  int y = pile->rule->y * (theme->height + theme->y_spacing);
  if (pile->rule->type == RULE_STOCK) {
    Card *top = get_top(pile->stack);
    top->up = 0;
    if (print_card_full(y, pile->rule->x, top, theme)) {
      cursor_pile = pile;
    }
  } else if (pile->stack->suit == TABLEAU) {
    if (print_tableau(y, pile->rule->x, pile->stack, theme)) {
      cursor_pile = pile;
    }
  } else if (print_stack(y, pile->rule->x, pile->stack, theme)) {
    cursor_pile = pile;
  }
}

static void ui_message(const char *format, ...) {
  va_list va;
  move(0, 0);
  clrtoeol();
  va_start(va, format);
  move(0, 0);
  vw_printw(stdscr, format, va);
  va_end(va);
}

static int ui_confirm(const char *message) {
  ui_message("%s (Y/N)", message);
  switch (getch()) {
    case 'y': case 'Y':
      return 1;
    default:
      return 0;
  }
}

static void ui_victory_banner(int y, int x) {
  attron(COLOR_PAIR(COLOR_PAIR_BACKGROUND));
  mvprintw(y    , x, "**************************************");
  mvprintw(y + 1, x, "*              VICTORY!              *");
  mvprintw(y + 2, x, "* Press 'r' to redeal or 'q' to quit *");
  mvprintw(y + 3, x, "**************************************");
}

int ui_victory(Pile *piles, Theme *theme) {
  int banner_y, banner_x;
  Pile *pile;
  Card *card;
  getmaxyx(stdscr, win_h, win_w);
  banner_y = win_h / 2 - 3;
  banner_x = win_w >= 38 ? win_w / 2 - 19 : 0;
  nodelay(stdscr, 1);
  for (pile = piles; pile; pile = pile->next) {
    int pile_y = pile->rule->y * (theme->height + theme->y_spacing);
    for (card = get_top(pile->stack); NOT_BOTTOM(card); card = card->prev) {
      double y, x, vy, vx;
      card->up = 1;
      y = (double)theme_y(pile_y, theme);
      x = (double)theme_x(pile->rule->x, theme);
      vy = (double)rand() / RAND_MAX * -4.0;
      vx = (double)rand() / RAND_MAX * 8.0 - 1.0;
      while (y < win_h) {
        switch (getch()) {
          case 'r':
            nodelay(stdscr, 0);
            return 1;
          case 'q':
            return 0;
        }
        print_card(y, x, card, 1, theme);
        ui_victory_banner(banner_y, banner_x);
        refresh();
        napms(70);
        y += vy;
        x += vx;
        vy += 0.5;
        if (x < 0) {
          x = 0;
          vx *= -1;
        } else if (x > win_w - theme->width) {
          x = win_w - theme->width;
          vx *= -1;
        }
      }
    }
  }
  while (1) {
    switch (getch()) {
      case 'r':
        nodelay(stdscr, 0);
        return 1;
      case 'q':
        return 0;
    }
  }
}

int ui_loop(Game *game, Theme *theme, Pile *piles) {
  MEVENT mouse;
  int move_made = 0;
  int mouse_action = 0;
  selection = NULL;
  selection_pile = NULL;
  clear();
  clear_undo_history();
  move_counter = 0;
  off_y = 0;
  wbkgd(stdscr, COLOR_PAIR(COLOR_PAIR_BACKGROUND));
  while (1) {
    Pile *pile;
    int ch;
    cursor_card = NULL;
    cursor_pile = NULL;
    max_x = max_y = 0;
    getmaxyx(stdscr, win_h, win_w);
    if (theme->y_margin + off_y + cur_y >= win_h) {
      clear();
      off_y = win_h - cur_y - theme->y_margin - 1;
    }
    if (theme->y_margin + off_y + cur_y < 0) {
      clear();
      off_y = -theme->y_margin - cur_y;
      if (cur_y == 0) {
        off_y = 0;
      }
    }
    for (pile = piles; pile; pile = pile->next) {
      print_pile(pile, theme);
    }
    move(theme->y_margin + off_y + cur_y, theme->x_margin + cur_x * (theme->width + theme->x_spacing));
    refresh();

    attron(COLOR_PAIR(COLOR_PAIR_BACKGROUND));

    if (move_made) {
      if (check_win_condition(piles)) {
        return ui_victory(piles, theme);
      }
      move_made = 0;
    }

    if (mouse_action) {
      ch = mouse_action;
      mouse_action = 0;
    } else {
      ch = getch();
    }
    switch (ch) {
      case 'h':
      case KEY_LEFT:
        cur_x--;
        if (cur_x < 0) cur_x = 0;
        break;
      case 'j':
      case KEY_DOWN:
        cur_y++;
        if (cur_y > max_y) cur_y = max_y;
        break;
      case 'k':
      case KEY_UP:
        cur_y--;
        if (cur_y < 0) cur_y = 0;
        break;
      case 'l':
      case KEY_RIGHT:
        cur_x++;
        if (cur_x > max_x) cur_x = max_x;
        break;
      case 'K':
#ifdef MSDOS
      case 547: /* shift-up */
#else
      case 337: /* shift-up */
#endif
        if (cursor_card) {
          if (cursor_card->prev && NOT_BOTTOM(cursor_card->prev)) {
            Card *card = cursor_card->prev;
            while (card->prev && card->prev->up == card->up && NOT_BOTTOM(card->prev)) {
              card = card->prev;
            }
            cur_y = card->y;
          } else {
            cur_y -= theme->height;
          }
        } else if (cursor_pile) {
          Card *card = get_top(cursor_pile->stack);
          cur_y = card->y;
        } else {
          cur_y -= theme->height;
        }
        if (cur_y < 0) cur_y = 0;
        break;
      case 'J':
#ifdef MSDOS
      case 548: /* shift-down */
#else
      case 336: /* shift-down */
#endif
        if (cursor_card && cursor_card->next) {
          Card *card = cursor_card->next;
          while (card->next && card->next->up == card->up) {
            card = card->next;
          }
          cur_y = card->y;
        } else {
          cur_y += theme->height;
        }
        if (cur_y > max_y) cur_y = max_y;
        break;
      case 'H':
#ifdef MSDOS
      case 391: /* shift-left */
#else
      case 393: /* shift-left */
#endif
        cur_x = 0;
        break;
      case 'L':
#ifdef MSDOS
      case 400: /* shift-right */
#else
      case 402: /* shift-right */
#endif
        cur_x = max_x;
        break;
      case ' ':
        if (cursor_card) {
          if (!(cursor_card->suit & BOTTOM)) {
            if (cursor_card->up) {
              if (selection == cursor_card) {
                if (move_to_foundation(cursor_card, cursor_pile, piles) || move_to_free_cell(cursor_card, cursor_pile, piles)) {
                  move_made = 1;
                  clear();
                  selection = NULL;
                  selection_pile = NULL;
                } else {
                  ui_message(get_move_error());
                }
              } else {
                selection = cursor_card;
                selection_pile = cursor_pile;
              }
            } else if (cursor_pile->rule->type == RULE_STOCK) {
              if (move_to_waste(cursor_card, cursor_pile, piles)) {
                move_made = 1;
                clear();
              } else {
                ui_message(get_move_error());
              }
            } else {
              turn_card(cursor_card);
            }
          } else if (cursor_pile->rule->type == RULE_STOCK) {
            if (redeal(cursor_pile, piles)) {
              move_made = 1;
              clear();
            } else {
              ui_message(get_move_error());
            }
          }
        }
        break;
      case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': {
        Pile *pile;
        int cell_i = ch - '0';
        for (pile = piles; pile; pile = pile->next) {
          if (pile->rule->type == RULE_CELL) {
            cell_i--;
            if (!cell_i) {
              Card *src = get_top(pile->stack);
              if (cursor_pile && NOT_BOTTOM(src)) {
                if (legal_move_stack(cursor_pile, src, pile, piles)) {
                  move_made = 1;
                  clear();
                } else {
                  ui_message(get_move_error());
                }
              }
              break;
            }
          }
        }
        break;
      }
      case 's': {
        Pile *pile;
        for (pile = piles; pile; pile = pile->next) {
          if (pile->rule->type == RULE_STOCK) {
            Card *src = get_top(pile->stack);
            if (IS_BOTTOM(src)) {
              if (redeal(pile, piles)) {
                move_made = 1;
                clear();
              } else {
                ui_message(get_move_error());
              }
            } else if (move_to_waste(src, pile, piles)) {
              move_made = 1;
              clear();
            } else {
              ui_message(get_move_error());
            }
            break;
          }
        }
        break;
      }
      case 'w': {
        Pile *pile;
        for (pile = piles; pile; pile = pile->next) {
          if (pile->rule->type == RULE_WASTE) {
            Card *src = get_top(pile->stack);
            if (cursor_pile && NOT_BOTTOM(src)) {
              if (legal_move_stack(cursor_pile, src, pile, piles)) {
                move_made = 1;
                clear();
              } else {
                ui_message(get_move_error());
              }
            }
            break;
          }
        }
        break;
      }
      case 'm':
      case 10: /* enter */
      case 13: /* enter */
        if (selection && cursor_pile) {
          if (legal_move_stack(cursor_pile, selection, selection_pile, piles)) {
            move_made = 1;
            clear();
            selection = NULL;
            selection_pile = NULL;
          } else {
            ui_message(get_move_error());
          }
        }
        break;
      case 'a':
        if (auto_move_to_foundation(piles)) {
          move_made = 1;
          clear();
        }
        break;
      case 'u':
      case 26: /* ^z */
        undo_move();
        clear();
        break;
      case 'U':
      case 25: /* ^y */
        redo_move();
        clear();
        break;
      case 27:
        clear();
        selection = NULL;
        selection_pile = NULL;
        break;
      case KEY_RESIZE:
        clear();
        break;
      case 'r':
        if (ui_confirm("Redeal?")) {
          return 1;
        }
        clear();
        break;
      case 'q':
        return 0;
      case KEY_MOUSE:
        if (
#ifdef MSDOS
            nc_getmouse(&mouse)
#else
            getmouse(&mouse)
#endif
            == OK) {
          cur_y = mouse.y - theme->y_margin - off_y;
          cur_x = (mouse.x - theme->x_margin) / (theme->width + theme->x_spacing);
          if (mouse.bstate & BUTTON3_PRESSED) {
            mouse_action = 'm';
          } else if (mouse.bstate & BUTTON1_PRESSED) {
            mouse_action = ' ';
          }
        }
        break;
      default:
        if (isgraph(ch)) {
          ui_message("Unbound key: %c (%d)", ch, ch);
        } else if (ch < ' ') {
          ui_message("Unbound key: ^%c", '@' + ch);
        } else {
          ui_message("Unbound key: (%d)", ch);
        }
        break;
    }
  }
  return 0;
}

short find_color(Theme *theme, short index, char *name) {
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

void find_and_init_color_pair(Theme *theme, short index, ColorPair color_pair) {
  init_pair(index, find_color(theme, color_pair.fg, color_pair.fg_name),
      find_color(theme, color_pair.bg, color_pair.bg_name));
}

void ui_main(Game *game, Theme *theme, int enable_color, unsigned int seed) {
  mmask_t oldmask;
  setlocale(LC_ALL, "");
  initscr();
  if (enable_color) {
    short index = 15;
    Color *color;
    start_color();
    for (color = theme->colors; color; color = color->next) {
      short color_index = color->name ? ++index : color->index;
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
  raw();
  clear();
  curs_set(1);
  keypad(stdscr, 1);
  noecho();

  mousemask(BUTTON1_PRESSED | BUTTON3_PRESSED, &oldmask);

  while (1) {
    Card *deck;
    Pile *piles;
    int redeal;
    srand(seed);

    deck = new_deck();
    move_stack(deck, shuffle_stack(take_stack(deck->next)));

    piles = deal_cards(game, deck);
    deals++;

    redeal = ui_loop(game, theme, piles);
    delete_piles(piles);
    delete_stack(deck);

    if (redeal) {
      seed = time(NULL) + deals;
    } else {
      break;
    }
  }

  endwin();
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
