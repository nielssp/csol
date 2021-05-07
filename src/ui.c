/* csol
 * Copyright (c) 2017 Niels Sonnich Poulsen (http://nielssp.dk)
 * Licensed under the MIT license.
 * See the LICENSE file or http://opensource.org/licenses/MIT for more information.
 */

#include "ui.h"

#include "rc.h"
#include "card.h"
#include "theme.h"
#include "scores.h"

#include <stdlib.h>
#ifdef USE_PDCURSES
#include <curses.h>
#else
#include <ncurses.h>
#endif
#include <locale.h>
#include <time.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

#ifdef PDCURSES
#define RAW_OUTPUT(n) raw_output(n)
#else
#define RAW_OUTPUT(n)
#define KEY_SUP    337
#define KEY_SDOWN  336
#endif

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

static void print_card_name_l(int y, int x, Card *card, Theme *theme) {
  if (y < 0 || y >= win_h) {
    return;
  }
  RAW_OUTPUT(1);
  mvprintw(y, x, "%s", theme->ranks[card->rank - 1]);
  printw("%s", card_suit(card, theme));
  RAW_OUTPUT(0);
}

static int utf8strlen(char *s) {
#if USE_PDCURSES
  return strlen(s);
#else
  int length = 0;
  while (*s) {
    length += ((*s >> 6) & 3) != 2;
    s++;
  }
  return length;
#endif
}

static void print_card_name_r(int y, int x, Card *card, Theme *theme) {
  char *suit_symbol, *rank_symbol;
  int width;
  if (y < 0 || y >= win_h) {
    return;
  }
  suit_symbol = card_suit(card, theme);
  rank_symbol = theme->ranks[card->rank - 1];
  width = utf8strlen(suit_symbol) + utf8strlen(rank_symbol);
  RAW_OUTPUT(1);
  mvprintw(y, x - width, "%s%s", suit_symbol, rank_symbol);
  RAW_OUTPUT(0);
}

static void print_text(int y, int x, Card *card, Text text, int fill, Theme *theme) {
  char *suit_symbol = "", *rank_symbol = "";
  if ((!fill && text.y != 0) || !text.format) {
    return;
  }
  if (text.y < 0) {
    y += theme->height + text.y;
  }
  if (text.x < 0) {
    x += theme->width + text.x;
  }
  if (y < 0 || y >= win_h) {
    return;
  }
  if (text.format == TEXT_RANK_SUIT || text.format == TEXT_SUIT_RANK
      || text.format == TEXT_SUIT) {
    suit_symbol = card_suit(card, theme);
  }
  if (text.format == TEXT_RANK_SUIT || text.format == TEXT_SUIT_RANK
      || text.format == TEXT_RANK) {
    rank_symbol = theme->ranks[card->rank - 1];
  }
  if (text.align_right) {
    x -= utf8strlen(suit_symbol) + utf8strlen(rank_symbol) - 1;
  }
  RAW_OUTPUT(1);
  if (text.format == TEXT_RANK_SUIT) {
    mvprintw(y, x, "%s%s", rank_symbol, suit_symbol);
  } else {
    mvprintw(y, x, "%s%s", suit_symbol, rank_symbol);
  }
  RAW_OUTPUT(0);
}

static void print_layout(int y, int x, Card *card, Layout layout, int full, Theme *theme) {
  Text *field;
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
  for (field = layout.text_fields; field; field = field->next) {
    print_text(y, x, card, *field, full, theme);
  }
}

static void print_card(int y, int x, Card *card, int full, Theme *theme) {
  if (card == selection) {
    attron(A_REVERSE);
  }
  if (card->suit & BOTTOM) {
    attron(COLOR_PAIR(COLOR_PAIR_EMPTY));
    print_layout(y, x, card, theme->empty_layout, full, theme);
    if (!theme->empty_layout.text_fields && card->rank > 0) {
      print_card_name_l(y, x + theme->empty_layout.left_padding, card, theme);
    }
  } else if (!card->up) {
    attron(COLOR_PAIR(COLOR_PAIR_BACK));
    print_layout(y, x, card, theme->back_layout, full, theme);
  } else {
    int left_padding, right_padding, has_text;
    if (card->suit & RED) {
      attron(COLOR_PAIR(COLOR_PAIR_RED));
      print_layout(y, x, card, theme->red_layout, full, theme);
      left_padding = theme->red_layout.left_padding;
      right_padding = theme->red_layout.right_padding;
      has_text = !!theme->red_layout.text_fields;
    } else {
      attron(COLOR_PAIR(COLOR_PAIR_BLACK));
      print_layout(y, x, card, theme->black_layout, full, theme);
      left_padding = theme->black_layout.left_padding;
      right_padding = theme->black_layout.right_padding;
      has_text = !!theme->black_layout.text_fields;
    }
    if (!has_text) {
      if (full && theme->height > 1) {
        print_card_name_r(y + theme->height - 1, x + theme->width - right_padding, card, theme);
      }
      print_card_name_l(y, x + left_padding, card, theme);
    }
  }
  attroff(A_REVERSE);
}

static int theme_y(int y, Theme *theme) {
  return theme->y_margin + off_y + y;
}

static int theme_x(int x, Theme *theme) {
  return theme->x_margin + x * (theme->width + theme->x_spacing);
}

static int print_card_in_grid(int y, int x, Card *card, int full, Theme *theme) {
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

static int print_card_top(int y, int x, Card *card, Theme *theme) {
  return print_card_in_grid(y, x, card, 0, theme);
}

static int print_card_full(int y, int x, Card *card, Theme *theme) {
  return print_card_in_grid(y, x, card, 1, theme);
}

static int print_stack(int y, int x, Card *bottom, Theme *theme) {
  if (bottom->next) {
    return print_stack(y, x, bottom->next, theme);
  } else {
    return print_card_full(y, x, bottom, theme);
  }
}

static int print_tableau(int y, int x, Card *bottom, Theme *theme) {
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

static void print_pile(Pile *pile, Theme *theme) {
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

static void ui_box(int y, int x, int height, int width, int fill) {
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

void format_time(char *out, int32_t time) {
  if (time > INT32_C(86400)) {
    sprintf(out, "%" PRId32 "d %02" PRId32 ":%02" PRId32 ":%02" PRId32,
        time / INT32_C(86400), (time / INT32_C(3600)) % INT32_C(24), (time / INT32_C(60)) % INT32_C(60), time % INT32_C(60));
  } else if (time > INT32_C(3600)) {
    sprintf(out, "%" PRId32 ":%02" PRId32 ":%02" PRId32,
        time / INT32_C(3600), (time / INT32_C(60)) % INT32_C(60), time % INT32_C(60));
  } else if (time >= INT32_C(0)) {
    sprintf(out, "%" PRId32 ":%02" PRId32, time / INT32_C(60), time % INT32_C(60));
  } else {
    sprintf(out, "n/a");
  }
}

static void ui_victory_banner(int y, int x, int32_t score, int32_t time, Stats stats) {
  char time_buffer[18];
  int height = 4;
  if (stats.times_played > 1 && stats.best_time >= 0) {
    height += 1;
  }
  attron(COLOR_PAIR(COLOR_PAIR_BACKGROUND));
  ui_box(y, x, height, 38, 1);
  format_time(time_buffer, time);
  mvprintw(y + 1, x + 2, "VICTORY!  Score: %" PRId32 " / %s", score, time_buffer);
  if (stats.times_played > 1 && stats.best_time >= 0) {
    format_time(time_buffer, stats.best_time);
    mvprintw(y + 2, x + 12, "Best:  %" PRId32 " / %s", stats.best_score,
        time_buffer);
  }
  mvprintw(y + height - 2, x + 2, "Press 'r' to redeal or 'q' to quit");
}

static int ui_victory(Pile *piles, Theme *theme, int32_t score, int32_t time, Stats stats) {
  int banner_y, banner_x;
  Pile *pile;
  Card *card;
  getmaxyx(stdscr, win_h, win_w);
  banner_y = win_h / 2 - 3;
  banner_x = win_w >= 38 ? win_w / 2 - 19 : 0;
  nodelay(stdscr, 1);
  curs_set(0);
  for (pile = piles; pile; pile = pile->next) {
    int pile_y = pile->rule->y * (theme->height + theme->y_spacing);
    for (card = get_top(pile->stack); NOT_BOTTOM(card); card = card->prev) {
      double y, x, vy, vx;
      card->up = 1;
      y = (double)theme_y(pile_y, theme);
      x = (double)theme_x(pile->rule->x, theme);
      vy = (double)rand() / RAND_MAX * -4.0;
      vx = (double)rand() / RAND_MAX * 8.0 - 4.0;
      while (y < win_h) {
        switch (getch()) {
          case 'r':
            nodelay(stdscr, 0);
            curs_set(1);
            return 1;
          case 'q':
            return 0;
        }
        print_card(y, x, card, 1, theme);
        ui_victory_banner(banner_y, banner_x, score, time, stats);
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
  nodelay(stdscr, 0);
  while (1) {
    switch (getch()) {
      case 'r':
        curs_set(1);
        return 1;
      case 'q':
        return 0;
    }
  }
}

static int ui_loop(Game *game, Theme *theme, Pile *piles) {
  MEVENT mouse;
  int move_made = 0;
  int mouse_action = 0;
  int game_started = 0;
  time_t start_time;
  selection = NULL;
  selection_pile = NULL;
  clear();
  clear_undo_history();
  move_counter = 0;
  game_score = 0;
  off_y = 0;
  wbkgd(stdscr, COLOR_PAIR(COLOR_PAIR_BACKGROUND));
  refresh();
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
    attron(COLOR_PAIR(COLOR_PAIR_BACKGROUND));
    if (show_score) {
      mvprintw(win_h - 1, 0, "Score: %d", game_score);
    }
    move(theme->y_margin + off_y + cur_y, theme->x_margin + cur_x * (theme->width + theme->x_spacing));
    refresh();

    if (move_made) {
      if (!game_started) {
        game_started = 1;
        start_time = time(NULL);
      }
      if (check_win_condition(piles)) {
        Stats stats;
        int32_t duration = difftime(time(NULL), start_time);
        append_score(game->name, 1, game_score, duration, &stats);
        return ui_victory(piles, theme, game_score, duration, stats);
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
      case KEY_SUP:
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
      case KEY_SDOWN:
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
      case KEY_SLEFT:
        cur_x = 0;
        break;
      case 'L':
      case KEY_SRIGHT:
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
              if (turn_from_stock(cursor_card, cursor_pile, piles)) {
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
            } else if (turn_from_stock(src, pile, piles)) {
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
      case 12: /* ^l */
        clear();
        break;
      case KEY_RESIZE:
        clear();
        break;
      case 'r':
        if (!game_started || ui_confirm("Redeal?")) {
          if (game_started) {
            append_score(game->name, 0, game_score, time(NULL) - start_time, NULL);
          }
          return 1;
        }
        clear();
        break;
      case 'q':
        if (!game_started || ui_confirm("Quit?")) {
          if (game_started) {
            append_score(game->name, 0, game_score, time(NULL) - start_time, NULL);
          }
          return 0;
        }
        clear();
        break;
      case KEY_MOUSE:
        if (
#ifdef PDCURSES
            nc_getmouse(&mouse)
#else
            getmouse(&mouse)
#endif
            == OK) {
          cur_y = mouse.y - theme->y_margin - off_y;
          cur_x = (mouse.x - theme->x_margin) / (theme->width + theme->x_spacing);
          if (mouse.bstate & BUTTON3_CLICKED) {
            mouse_action = 'm';
          } else if (mouse.bstate & BUTTON1_CLICKED) {
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

#ifdef USE_PDCURSES

static unsigned char convert_code_point(int code_point) {
  switch (code_point) {
    case 0x263A: return 1;
    case 0x263B: return 2;
    case 0x2665: return 3;
    case 0x2666: return 4;
    case 0x2663: return 5;
    case 0x2660: return 6;
    case 0x2022: return 7;
    case 0x25D8: return 8;
    case 0x25CB: return 9;
    case 0x25D9: return 10;
    case 0x2642: return 11;
    case 0x2640: return 12;
    case 0x266A: return 13;
    case 0x266B: return 14;
    case 0x263C: return 15;
    case 0x25BA: return 16;
    case 0x25C4: return 17;
    case 0x2195: return 18;
    case 0x203C: return 19;
    case 0x00B6: return 20;
    case 0x00A7: return 21;
    case 0x25AC: return 22;
    case 0x21A8: return 23;
    case 0x2191: return 24;
    case 0x2193: return 25;
    case 0x2192: return 26;
    case 0x2190: return 27;
    case 0x221F: return 28;
    case 0x2194: return 29;
    case 0x25B2: return 30;
    case 0x25BC: return 31;
    case 0x2302: return 127;
    case 0x2591: return 176;
    case 0x2592: return 177;
    case 0x2593: return 178;
    case 0x2502: return 179;
    case 0x2524: return 180;
    case 0x2561: return 181;
    case 0x2562: return 182;
    case 0x2556: return 183;
    case 0x2555: return 184;
    case 0x2563: return 185;
    case 0x2551: return 186;
    case 0x2557: return 187;
    case 0x255D: return 188;
    case 0x255C: return 189;
    case 0x255B: return 190;
    case 0x2510: return 191;
    case 0x2514: return 192;
    case 0x2534: return 193;
    case 0x252C: return 194;
    case 0x251C: return 195;
    case 0x2500: return 196;
    case 0x253C: return 197;
    case 0x255E: return 198;
    case 0x255F: return 199;
    case 0x255A: return 200;
    case 0x2554: return 201;
    case 0x2569: return 202;
    case 0x2566: return 203;
    case 0x2560: return 204;
    case 0x2550: return 205;
    case 0x256C: return 206;
    case 0x2567: return 207;
    case 0x2568: return 208;
    case 0x2564: return 209;
    case 0x2565: return 210;
    case 0x2559: return 211;
    case 0x2558: return 212;
    case 0x2552: return 213;
    case 0x2553: return 214;
    case 0x256B: return 215;
    case 0x256A: return 216;
    case 0x2518: return 217;
    case 0x250C: return 218;
    case 0x2588: return 219;
    case 0x2584: return 220;
    case 0x258C: return 221;
    case 0x2590: return 222;
    case 0x2580: return 223;
    default:
      printf("Unknown code point: %04x\n", code_point);
      return '?';
  }
}

static void convert_string(char *str) {
  char *s;
  int next = 0;
  int code_point = 0;
  if (!str) {
    return;
  }
  for (s = str ; *s; s++) {
    if (*s & 0x80) {
      if (*s & 0x40) {
        if (code_point) {
          str[next++] = convert_code_point(code_point);
          code_point = 0;
        }
        if ((*s & 0xE0) == 0xC0) {
          code_point = *s & 0x1F;
        } else if ((*s & 0xF0) == 0xE0) {
          code_point = *s & 0x0F;
        } else if ((*s & 0xF8) == 0xF0) {
          code_point = *s & 0x07;
        }
      } else {
        code_point <<= 6;
        code_point |= *s & 0x3F;
      }
    } else {
      if (code_point) {
        str[next++] = convert_code_point(code_point);
        code_point = 0;
      }
      str[next++] = *s;
    }
  }
  if (code_point) {
    str[next++] = convert_code_point(code_point);
    code_point = 0;
  }
  str[next] = 0;
}

static void convert_layout(Layout *layout) {
  convert_string(layout->top);
  convert_string(layout->middle);
  convert_string(layout->bottom);
}

static void convert_theme(Theme *theme) {
  theme->utf8 = 0;
  convert_string(theme->heart);
  convert_string(theme->spade);
  convert_string(theme->diamond);
  convert_string(theme->club);
  convert_layout(&theme->empty_layout);
  convert_layout(&theme->back_layout);
  convert_layout(&theme->red_layout);
  convert_layout(&theme->black_layout);
}

#endif

void ui_main(Game *game, Theme *theme, int enable_color, unsigned int seed) {
#ifdef USE_PDCURSES
  if (theme->utf8) {
    printf("Converting UTF8 theme\n");
    convert_theme(theme);
  }
#endif
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

  mousemask(BUTTON1_CLICKED | BUTTON3_CLICKED, NULL);

  while (1) {
    Card *deck;
    Pile *piles;
    int redeal;
    srand(seed);

    deck = new_deck(game->decks, game->deck_suits);
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
