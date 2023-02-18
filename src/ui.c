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
#include "menu.h"
#include "color.h"
#include "config.h"

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

int deals = 0;

int cur_x = 0;
int cur_y = 0;

int max_cur_y = 0;

int max_x = 0;
int max_y = 0;

int win_w = 0;
int win_h = 0;

int off_y = 0;

Card *selection = NULL;
Pile *selection_pile = NULL;
Card *cursor_card = NULL;
Pile *cursor_pile = NULL;

/* Next card in all four directions for smart cursor movement */
Card *n_card = NULL;
Card *e_card = NULL;
Card *s_card = NULL;
Card *w_card = NULL;
/* Next pile in all four directions for smart cursor movement */
Pile *n_pile = NULL;
Pile *e_pile = NULL;
Pile *s_pile = NULL;
Pile *w_pile = NULL;
/* Left- and rightmost cards for smart cursor movement */
Card *em_card = NULL;
Card *wm_card = NULL;
Pile *em_pile = NULL;
Pile *wm_pile = NULL;

enum {
  ACTION_RESTART = 1,
  ACTION_SAVE_CONFIG,
  ACTION_QUIT,
  ACTION_UNDO,
  ACTION_REDO,
  ACTION_AUTO,
  ACTION_STOCK,
  ACTION_WASTE,
  ACTION_SMART_CURSOR,
  ACTION_VERTICAL_STABILIZATION,
  ACTION_CHANGE_CURSOR,
  ACTION_SHOW_SCORE,
  ACTION_SHOW_MENUBAR,
  ACTION_HOW_TO_PLAY,
  ACTION_ABOUT
};

Menu file_menu[] = {
  {"&Restart", "r", ACTION_RESTART, NULL, NULL},
  {"&Save config", NULL, ACTION_SAVE_CONFIG, NULL, NULL},
  {"&Quit", "q", ACTION_QUIT, NULL, NULL},
  {NULL, NULL, 0, NULL, NULL}
};

Menu move_menu[] = {
  {"&Undo", "u", ACTION_UNDO, NULL, NULL},
  {"&Redo", "U", ACTION_REDO, NULL, NULL},
  {"&Auto", "a", ACTION_AUTO, NULL, NULL},
  {"From &Stock", "s", ACTION_STOCK, NULL, NULL},
  {"From &Waste", "w", ACTION_WASTE, NULL, NULL},
  {NULL, NULL, 0, NULL, NULL}
};

Menu settings_menu[] = {
  {"Smart &cursor", "^S", ACTION_SMART_CURSOR, NULL, NULL},
  {"&Vertical stabilization", "^V", ACTION_VERTICAL_STABILIZATION, NULL, NULL},
  {"&Change cursor", NULL, ACTION_CHANGE_CURSOR, NULL, NULL},
  {"Show &score", "", ACTION_SHOW_SCORE, NULL, NULL},
  {"Show &menubar", "", ACTION_SHOW_MENUBAR, NULL, NULL},
  {NULL, NULL, 0, NULL, NULL}
};

Menu help_menu[] = {
  {"&How to play", "?", ACTION_HOW_TO_PLAY, NULL, NULL},
  {"&About csol", "", ACTION_ABOUT, NULL, NULL},
  {NULL, NULL, 0, NULL, NULL}
};

Menu main_menu[] = {
  {"&File", NULL, 0, NULL, file_menu},
  {"&Move", NULL, 0, NULL, move_menu},
  {"&Game", NULL, 0, NULL, game_menu},
  {"&Theme", NULL, 0, NULL, theme_menu},
  {"&Settings", NULL, 0, NULL, settings_menu},
  {"&Help", NULL, 0, NULL, help_menu},
  {NULL, NULL, 0, NULL, NULL}
};

Menu *menu_selection[] = {
  NULL,
  NULL
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
  if (card == selection) {
    attroff(A_REVERSE);
  }
}

static int theme_y(int y, Theme *theme) {
  return theme->y_margin + off_y + y;
}

static int theme_x(int x, Theme *theme) {
  return theme->x_margin + x * (theme->width + theme->x_spacing);
}

static void update_directions(Card *card, int y_max) {
  if (!card->up && card->next) {
    return;
  }
  if (card->x == cur_x) {
    if (card->y < cur_y && (card->next || y_max < cur_y)) {
      if (!n_card || card->y > n_card->y) {
        n_card = card;
      }
    } else if (card->y > cur_y) {
      if (!s_card || card->y < s_card->y) {
        s_card = card;
      }
    }
  }
  if (keep_vertical_position
      ? card->y <= cur_y && y_max >= cur_y
      : card->y <= max_cur_y && y_max >= max_cur_y) {
    if (card->x < cur_x) {
      if (!w_card || card->x > w_card->x) {
        w_card = card;
      }
      if (!wm_card || card->x < wm_card->x) {
        wm_card = card;
      }
    } else if (card->x > cur_x) {
      if (!e_card || card->x < e_card->x) {
        e_card = card;
      }
      if (!em_card || card->x > em_card->x) {
        em_card = card;
      }
    }
  }
}

static int print_card_in_grid(int y, int x, Card *card, int full, Theme *theme) {
  int y2 = y + full * (theme->height - 1);
  if (y2 > max_y) max_y = y2;
  if (x > max_x) max_x = x;
  if (y <= cur_y && y2 >= cur_y && x == cur_x) cursor_card = card;
  card->x = x;
  card->y = y;
  update_directions(card, y2);
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
    } else if (pile->rule->x == cur_x) {
      if (y < cur_y && (!n_pile || n_pile->rule->y < pile->rule->y)) {
        n_pile = pile;
      } else if (y > cur_y && (!s_pile || s_pile->rule->y > pile->rule->y)) {
        s_pile = pile;
      }
    }
  } else if (pile->stack->suit == TABLEAU) {
    if (print_tableau(y, pile->rule->x, pile->stack, theme)) {
      cursor_pile = pile;
    } else if (cur_y >= y) {
      if (pile->rule->x < cur_x) {
        if (!w_pile || w_pile->rule->x < pile->rule->x) {
          w_pile = pile;
        }
        if (!wm_pile || wm_pile->rule->x > pile->rule->x) {
          wm_pile = pile;
        }
      } else if (pile->rule->x > cur_x) {
        if (!e_pile || e_pile->rule->x > pile->rule->x) {
          e_pile = pile;
        }
        if (!em_pile || em_pile->rule->x < pile->rule->x) {
          em_pile = pile;
        }
      }
    } else if (pile->rule->x == cur_x) {
      s_pile = pile;
    }
  } else if (print_stack(y, pile->rule->x, pile->stack, theme)) {
    cursor_pile = pile;
  } else if (pile->rule->x == cur_x) {
    if (y < cur_y && (!n_pile || n_pile->rule->y < pile->rule->y)) {
      n_pile = pile;
    } else if (y > cur_y && (!s_pile || s_pile->rule->y > pile->rule->y)) {
      s_pile = pile;
    }
  }
}

void format_time(char *out, int32_t time) {
  if (time > INT32_C(86400)) {
    sprintf(out, "%" PRId32 "d %02" PRId32 ":%02" PRId32 ":%02" PRId32,
        time / INT32_C(86400), (time / INT32_C(3600)) % INT32_C(24), (time / INT32_C(60)) % INT32_C(60),
        time % INT32_C(60));
  } else if (time > INT32_C(3600)) {
    sprintf(out, "%" PRId32 ":%02" PRId32 ":%02" PRId32,
        time / INT32_C(3600), (time / INT32_C(60)) % INT32_C(60), time % INT32_C(60));
  } else if (time >= INT32_C(0)) {
    sprintf(out, "%" PRId32 ":%02" PRId32, time / INT32_C(60), time % INT32_C(60));
  } else {
    sprintf(out, "n/a");
  }
}

static void how_to_play() {
  int y, x;
  int height = 7;
  int width = 60;
  getmaxyx(stdscr, win_h, win_w);
  y = win_h / 2 - 3;
  x = win_w >= width ? win_w / 2 - width / 2 : 0;
  attron(COLOR_PAIR(COLOR_PAIR_BACKGROUND));
  ui_box(y, x, height, width, 1);
  mvprintw(y + 1, x + 2, "Use the arrow keys or h, j, k, and l to move the cursor.");
  mvprintw(y + 2, x + 2, "Press space to select the card under the cursor.");
  mvprintw(y + 3, x + 2, "With a card selected, move the cursor again and press");
  mvprintw(y + 4, x + 2, "Enter or m to move the selected card to the position");
  mvprintw(y + 5, x + 2, "under the cursor.");
  getch();
}

static void about() {
  int y, x;
  int height = 5;
  int width = 50;
  getmaxyx(stdscr, win_h, win_w);
  y = win_h / 2 - 3;
  x = win_w >= width ? win_w / 2 - width / 2 : 0;
  attron(COLOR_PAIR(COLOR_PAIR_BACKGROUND));
  ui_box(y, x, height, width, 1);
  mvprintw(y + 1, x + 2, "csol " CSOL_VERSION);
  mvprintw(y + 2, x + 2, "Copyright (c) 2017-2023 Niels Sonnich Poulsen");
  mvprintw(y + 3, x + 2, "https://nielssp.dk/csol");
  getch();
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
            curs_set(!alt_cursor);
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
        curs_set(!alt_cursor);
        return 1;
      case 'q':
        return 0;
    }
  }
}

static int ui_loop(Game **current_game, Theme **current_theme, Pile *piles) {
  MEVENT mouse;
  MenuClick menu_click = {0, 0, 0};
  int new_game = 1;
  int move_made = 0;
  int mouse_action = 0;
  int game_started = 0;
  time_t start_time;
  int old_cur_x = 0;
  int old_cur_y = 0;
  void *menu_data = NULL;
  Game *game = *current_game;
  Theme *theme = *current_theme;
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
    n_card = e_card = s_card = w_card = NULL;
    n_pile = e_pile = s_pile = w_pile = NULL;
    em_card = wm_card = NULL;
    em_pile = wm_pile = NULL;
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
    if (new_game) {
      new_game = 0;
      if (smart_cursor && !cursor_card) {
        if (s_card) {
          cur_x = s_card->x;
          max_cur_y = cur_y = s_card->y;
          continue;
        } else if (e_card) {
          cur_x = e_card->x;
          max_cur_y = cur_y = e_card->y;
          continue;
        }
      }
    }
    if (smart_cursor) {
      if (!keep_vertical_position) {
        if (cursor_card && cursor_card->y < cur_y) {
          cur_y = cursor_card->y;
        } else if (cursor_card && cursor_card->next && cur_y < max_cur_y) {
          while (cursor_card->next && cursor_card->y < max_cur_y) {
            cursor_card = cursor_card->next;
          }
          cur_x = cursor_card->x;
          cur_y = cursor_card->y;
          continue; /* Necessary because the cursor card has changed */
        } else if (!cursor_card && n_card) {
          cur_x = n_card->x;
          cur_y = n_card->y;
        }
      }
      if (!cursor_card && !n_card && !s_card && !w_card && !e_card) {
        cur_x = 0;
        cur_y = 0;
        new_game = 1;
        mouse_action = 0;
        continue;
      }
    }
    if (alt_cursor) {
      mvprintw(theme->y_margin + off_y + old_cur_y,
          theme->x_margin + old_cur_x * (theme->width + theme->x_spacing) - 1, " ");
      mvprintw(theme->y_margin + off_y + old_cur_y,
          theme->x_margin + old_cur_x * (theme->width + theme->x_spacing) + theme->width, " ");
      refresh();
      mvprintw(theme->y_margin + off_y + cur_y,
          theme->x_margin + cur_x * (theme->width + theme->x_spacing) - 1, ">");
      mvprintw(theme->y_margin + off_y + cur_y,
          theme->x_margin + cur_x * (theme->width + theme->x_spacing) + theme->width, "<");
      old_cur_x = cur_x;
      old_cur_y = cur_y;
    }

    switch (ui_menubar(main_menu, menu_selection, &menu_data, &menu_click)) {
      case MENU_IS_CLOSED:
        break;
      case ACTION_RESTART:
        mouse_action = 'r';
        continue;
      case ACTION_SAVE_CONFIG:
        save_config(theme, game);
        continue;
      case ACTION_QUIT:
        mouse_action = 'q';
        continue;
      case ACTION_UNDO:
        mouse_action = 'u';
        continue;
      case ACTION_REDO:
        mouse_action = 'U';
        continue;
      case ACTION_AUTO:
        mouse_action = 'a';
        continue;
      case ACTION_STOCK:
        mouse_action = 's';
        continue;
      case ACTION_WASTE:
        mouse_action = 'w';
        continue;
      case ACTION_GAME:
        if (!game_started || ui_confirm("Redeal?")) {
          if (game_started) {
            append_score(game->name, 0, game_score, time(NULL) - start_time, NULL);
          }
          *current_game = menu_data;
          return 1;
        }
        clear();
        continue;
      case ACTION_THEME:
        restore_colors(theme);
        *current_theme = theme = menu_data;
        convert_theme(theme);
        init_theme_colors(theme);
        if (show_menu && theme->y_margin < 2) {
          theme->y_margin = 2;
        }
        clear();
        continue;
      case ACTION_SMART_CURSOR:
        smart_cursor = !smart_cursor;
        continue;
      case ACTION_VERTICAL_STABILIZATION:
        keep_vertical_position = !keep_vertical_position;
        continue;
      case ACTION_CHANGE_CURSOR:
        alt_cursor = !alt_cursor;
        curs_set(!alt_cursor);
        clear();
        continue;
      case ACTION_SHOW_SCORE:
        show_score = !show_score;
        clear();
        continue;
      case ACTION_SHOW_MENUBAR:
        show_menu = !show_menu;
        clear();
        continue;
      case ACTION_HOW_TO_PLAY:
        mouse_action = '?';
        continue;
      case ACTION_ABOUT:
        mouse_action = KEY_F(13);
        continue;
      default:
        continue;
    }
    refresh();

    if (!alt_cursor) {
      move(theme->y_margin + off_y + cur_y, theme->x_margin + cur_x * (theme->width + theme->x_spacing));
    }

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
        if (smart_cursor) {
          if (w_pile && (!w_card || w_card->x < w_pile->rule->x)) {
            if (keep_vertical_position) {
              cur_x = w_pile->rule->x;
            } else {
              Card *card = w_pile->stack;
              while (card->next && (IS_BOTTOM(card) || !card->up || card->y < max_cur_y)) {
                card = card->next;
              }
              cur_x = card->x;
              cur_y = card->y;
            }
          } else if (w_card) {
            cur_x = w_card->x;
            if (!keep_vertical_position) {
              cur_y = w_card->y;
            }
          }
        } else {
          cur_x--;
          if (cur_x < 0) cur_x = 0;
        }
        break;
      case 'j':
      case KEY_DOWN:
        if (smart_cursor) {
          if (s_card) {
            cur_x = s_card->x;
            cur_y = s_card->y;
            max_cur_y = cur_y;
          }
        } else {
          cur_y++;
          if (cur_y > max_y) cur_y = max_y;
        }
        break;
      case 'k':
      case KEY_UP:
        if (smart_cursor) {
          if (n_card) {
            cur_x = n_card->x;
            cur_y = n_card->y;
            max_cur_y = cur_y;
          } else if (off_y < 0) {
            off_y++;
            clear();
          }
        } else {
          cur_y--;
          if (cur_y < 0) cur_y = 0;
        }
        break;
      case 'l':
      case KEY_RIGHT:
        if (smart_cursor) {
          if (e_pile && (!e_card || e_card->x > e_pile->rule->x)) {
            if (keep_vertical_position) {
              cur_x = e_pile->rule->x;
            } else {
              Card *card = e_pile->stack;
              while (card->next && (IS_BOTTOM(card) || !card->up || card->y < max_cur_y)) {
                card = card->next;
              }
              cur_x = card->x;
              cur_y = card->y;
            }
          } else if (e_card) {
            cur_x = e_card->x;
            if (!keep_vertical_position) {
              cur_y = e_card->y;
            }
          }
        } else {
          cur_x++;
          if (cur_x > max_x) cur_x = max_x;
        }
        break;
      case 'K':
      case KEY_SUP:
        if (cursor_card) {
          if (cursor_card->prev && NOT_BOTTOM(cursor_card->prev) && cursor_card->prev->up) {
            Card *card = cursor_card->prev;
            while (card->prev && card->prev->up == card->up && NOT_BOTTOM(card->prev)) {
              card = card->prev;
            }
            cur_y = card->y;
          } else if (n_pile) {
            Card *card = get_top(n_pile->stack);
            cur_x = card->x;
            cur_y = card->y;
          } else if (n_card) {
            cur_x = n_card->x;
            cur_y = n_card->y;
          } else if (!smart_cursor) {
            cur_y -= theme->height;
          }
        } else if (cursor_pile) {
          Card *card = get_top(cursor_pile->stack);
          cur_y = card->y;
        } else if (n_card) {
          cur_x = n_card->x;
          cur_y = n_card->y;
        } else if (!smart_cursor) {
          cur_y -= theme->height;
        }
        if (cur_y < 0) cur_y = 0;
        max_cur_y = cur_y;
        break;
      case 'J':
      case KEY_SDOWN:
        if (cursor_card && cursor_card->next) {
          Card *card;
          if (cursor_card->up) {
            card = get_top(cursor_card);
          } else {
            card = cursor_card->next;
            while (!card->up && card->next) {
              card = card->next;
            }
          }
          cur_y = card->y;
        } else if (s_pile) {
          Card *card = s_pile->stack;
          while ((IS_BOTTOM(card) || !card->up) && card->next) {
            card = card->next;
          }
          cur_y = card->y;
        } else if (s_card) {
          cur_x = s_card->x;
          cur_y = s_card->y;
        } else if (!smart_cursor) {
          cur_y += theme->height;
        }
        if (cur_y > max_y) cur_y = max_y;
        max_cur_y = cur_y;
        break;
      case 'H':
      case KEY_SLEFT:
        if (smart_cursor) {
          if (wm_pile && (!wm_card || wm_card->x > wm_pile->rule->x)) {
            if (keep_vertical_position) {
              cur_x = wm_pile->rule->x;
            } else {
              Card *card = wm_pile->stack;
              while (card->next && (IS_BOTTOM(card) || !card->up || card->y < cur_y)) {
                card = card->next;
              }
              cur_x = card->x;
              cur_y = card->y;
            }
          } else if (wm_card) {
            cur_x = wm_card->x;
            if (!keep_vertical_position) {
              cur_y = wm_card->y;
            }
          }
        } else {
          cur_x = 0;
        }
        break;
      case 'L':
      case KEY_SRIGHT:
        if (smart_cursor) {
          if (em_pile && (!em_card || em_card->x < em_pile->rule->x)) {
            if (keep_vertical_position) {
              cur_x = em_pile->rule->x;
            } else {
              Card *card = em_pile->stack;
              while (card->next && (IS_BOTTOM(card) || !card->up || card->y < cur_y)) {
                card = card->next;
              }
              cur_x = card->x;
              cur_y = card->y;
            }
          } else if (em_card) {
            cur_x = em_card->x;
            if (!keep_vertical_position) {
              cur_y = em_card->y;
            }
          }
        } else {
          cur_x = max_x;
        }
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
      case 18: /* ^r */
        redo_move();
        clear();
        break;
      case KEY_F(10):
        menu_selection[0] = main_menu;
        break;
      case 27:
        selection = NULL;
        selection_pile = NULL;
        open_menu(getch(), main_menu, menu_selection);
        clear();
        break;
      case 19: /* ^s */
        smart_cursor = !smart_cursor;
        if (smart_cursor) {
          ui_message("Smart cursor: Enabled");
        } else {
          ui_message("Smart cursor: Disabled");
        }
        break;
      case 22: /* ^v */
        keep_vertical_position = !keep_vertical_position;
        if (keep_vertical_position) {
          ui_message("Keep vertical position: Enabled");
        } else {
          ui_message("Keep vertical position: Disabled");
        }
        break;
      case 12: /* ^l */
        clear();
        break;
      case KEY_RESIZE:
        clear();
        break;
      case KEY_F(1):
      case '?':
        how_to_play();
        clear();
        break;
      case KEY_F(13):
        about();
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
          max_cur_y = cur_y;
          if (mouse.bstate & BUTTON3_CLICKED) {
            mouse_action = 'm';
          } else if (mouse.bstate & BUTTON1_CLICKED) {
            mouse_action = ' ';
            menu_click.click = 1;
            menu_click.x = mouse.x;
            menu_click.y = mouse.y;
          }
        }
        break;
      default:
        if (ch >= 417 && ch <= 442) {
          open_menu('a' + ch - 417, main_menu, menu_selection);
        } else if (isgraph(ch)) {
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

void ui_main(Game *game, Theme *theme, int enable_color, unsigned int seed) {
#ifdef USE_PDCURSES
  if (theme->utf8) {
    printf("Converting UTF8 theme\n");
    convert_theme(theme);
  }
#endif
  if (show_menu && theme->y_margin < 2) {
    theme->y_margin = 2;
  }
  setlocale(LC_ALL, "");
  initscr();
  if (enable_color) {
    start_color();
    init_theme_colors(theme);
  }
  raw();
  clear();
  curs_set(!alt_cursor);
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

    redeal = ui_loop(&game, &theme, piles);
    delete_piles(piles);
    delete_stack(deck);

    if (redeal) {
      seed = time(NULL) + deals;
    } else {
      break;
    }
  }
  if (enable_color) {
    restore_colors(theme);
  }
  endwin();
}
