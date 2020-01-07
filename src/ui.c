/* csol
 * Copyright (c) 2017 Niels Sonnich Poulsen (http://nielssp.dk)
 * Licensed under the MIT license.
 * See the LICENSE file or http://opensource.org/licenses/MIT for more information.
 */

#include <stdlib.h>
#include <ncurses.h>
#include <locale.h>
#include <time.h>
#include <ctype.h>

#include "ui.h"
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
  printw(card_suit(card, theme));
}

void print_card_name_r(int y, int x, Card *card, Theme *theme) {
  if (y < 0 || y >= win_h) {
    return;
  }
  mvprintw(y, x - 1 - (card->rank == 10), card_suit(card, theme));
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
    for (int i = 1; i < theme->height - 1; i++) {
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
      print_card_name_r(y + theme->height - 1, x + theme->width - (right_padding - 1), card, theme);
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
  getmaxyx(stdscr, win_h, win_w);
  int banner_y = win_h / 2 - 3;
  int banner_x = win_w >= 38 ? win_w / 2 - 19 : 0;
  nodelay(stdscr, 1);
  for (Pile *pile = piles; pile; pile = pile->next) {
    int pile_y = pile->rule->y * (theme->height + theme->y_spacing);
    for (Card *card = get_top(pile->stack); NOT_BOTTOM(card); card = card->prev) {
      card->up = 1;
      double y = theme_y(pile_y, theme);
      double x = theme_x(pile->rule->x, theme);
      double vy = (double)rand() / RAND_MAX * -4.0;
      double vx = (double)rand() / RAND_MAX * 8.0 - 1.0;
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
    for (Pile *pile = piles; pile; pile = pile->next) {
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

    int ch;
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
      case 337: // shift-up
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
      case 336: // shift-down
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
      case 393: // shift-left
        cur_x = 0;
        break;
      case 'L':
      case 402: // shift-right
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
        int cell_i = ch - '0';
        for (Pile *pile = piles; pile; pile = pile->next) {
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
      case 'm':
      case 10: // enter
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
      case 26: // ^z
        undo_move();
        clear();
        break;
      case 'U':
      case 25: // ^y
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
        if (getmouse(&mouse) == OK) {
          cur_y = mouse.y - theme->y_margin - off_y;
          cur_x = (mouse.x - theme->x_margin) / (theme->width + theme->x_spacing);
          if (mouse.bstate & BUTTON3_PRESSED) {
            mouse_action = 'm';
          } else {
            mouse_action = ' ';
          }
        }
        break;
      default:
        if (isgraph(ch)) {
          ui_message("Unbound key: %c", ch);
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
  setlocale(LC_ALL, "");
  initscr();
  if (enable_color) {
    start_color();
    for (Color *color = theme->colors; color; color = color->next) {
      if (init_color(color->index, color->red, color->green, color->blue) != 0) {
        // TODO: inform user
      }
    }
    init_pair(COLOR_PAIR_BACKGROUND, theme->background.fg, theme->background.bg);
    init_pair(COLOR_PAIR_EMPTY, theme->empty_layout.color.fg, theme->empty_layout.color.bg);
    init_pair(COLOR_PAIR_BACK, theme->back_layout.color.fg, theme->back_layout.color.bg);
    init_pair(COLOR_PAIR_RED, theme->red_layout.color.fg, theme->red_layout.color.bg);
    init_pair(COLOR_PAIR_BLACK, theme->black_layout.color.fg, theme->black_layout.color.bg);
  }
  raw();
  clear();
  curs_set(1);
  keypad(stdscr, 1);
  noecho();

  mmask_t oldmask;
  mousemask(BUTTON1_PRESSED | BUTTON3_PRESSED, &oldmask);

  while (1) {
    srand(seed);

    Card *deck = new_deck();
    move_stack(deck, shuffle_stack(take_stack(deck->next)));

    Pile *piles = deal_cards(game, deck);
    deals++;

    int redeal = ui_loop(game, theme, piles);
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
