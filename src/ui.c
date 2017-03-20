/* yuk
 * Copyright (c) 2017 Niels Sonnich Poulsen (http://nielssp.dk)
 * Licensed under the MIT license.
 * See the LICENSE file or http://opensource.org/licenses/MIT for more information.
 */

#include <stdlib.h>
#include <ncurses.h>
#include <locale.h>
#include <time.h>

#include "ui.h"
#include "card.h"
#include "theme.h"

#define COLOR_PAIR_EMPTY 1
#define COLOR_PAIR_BACK 2
#define COLOR_PAIR_RED 3
#define COLOR_PAIR_BLACK 4

int cur_x = 0;
int cur_y = 0;

int max_x = 0;
int max_y = 0;

Card *selection = NULL;
Pile *selection_pile = NULL;
Card *cursor_card = NULL;
Pile *cursor_pile = NULL;

void print_card_name_l(int y, int x, Card *card, Theme *theme) {
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
  mvprintw(y, x, layout.top);
  if (full && theme->height > 1) {
    for (int i = 1; i < theme->height; i++) {
      mvprintw(y + i, x, layout.middle);
    }
    mvprintw(y + theme->height - 1, x, layout.bottom);
  }
}

int print_card(int y, int x, Card *card, int full, Theme *theme) {
  int y2 = y + full * (theme->height - 1);
  if (y2 > max_y) max_y = y2;
  if (x > max_x) max_x = x;
  if (y <= cur_y && y2 >= cur_y && x == cur_x) cursor_card = card;
  y = theme->y_margin + y;
  x = theme->x_margin + x * (theme->width + theme->x_spacing);
  if (card == selection) {
    attron(A_REVERSE);
  }
  if (card->suit & BOTTOM) {
    attron(COLOR_PAIR(COLOR_PAIR_EMPTY));
    print_layout(y, x, card, theme->empty_layout, full, theme);
    if (card->rank > 0) {
      print_card_name_l(y, x + 1, card, theme);
    }
  } else if (!card->up) {
    attron(COLOR_PAIR(COLOR_PAIR_BACK));
    print_layout(y, x, card, theme->back_layout, full, theme);
  } else {
    if (card->suit & RED) {
      attron(COLOR_PAIR(COLOR_PAIR_RED));
      print_layout(y, x, card, theme->red_layout, full, theme);
    } else {
      attron(COLOR_PAIR(COLOR_PAIR_BLACK));
      print_layout(y, x, card, theme->black_layout, full, theme);
    }
    if (full && theme->height > 1) {
      print_card_name_r(y + theme->height - 1, x + theme->width - 2, card, theme);
    }
    print_card_name_l(y, x + 1, card, theme);
  }
  attroff(A_REVERSE);
  return cursor_card == card;
}

int print_card_top(int y, int x, Card *card, Theme *theme) {
  return print_card(y, x, card, 0, theme);
}

int print_card_full(int y, int x, Card *card, Theme *theme) {
  return print_card(y, x, card, 1, theme);
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

int ui_loop(Game *game, Theme *theme, Pile *piles) {
  selection = NULL;
  selection_pile = NULL;
  clear();
  while (1) {
    cursor_card = NULL;
    cursor_pile = NULL;
    max_x = max_y = 0;
    for (Pile *pile = piles; pile; pile = pile->next) {
      print_pile(pile, theme);
    }
    move(theme->y_margin + cur_y, theme->x_margin + cur_x * (theme->width + theme->x_spacing));
    refresh();

    int ch = getch();
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
      case ' ':
        if (cursor_card) {
          if (!(cursor_card->suit & BOTTOM)) {
            if (cursor_card->up) {
              selection = cursor_card;
              selection_pile = cursor_pile;
            } else if (cursor_pile->rule->type == RULE_STOCK) {
              if (move_to_waste(cursor_card, cursor_pile, piles)) {
                clear();
              }
            } else if (!cursor_card->next) {
              cursor_card->up = 1;
            }
          } else if (cursor_pile->rule->type == RULE_STOCK) {
            if (redeal(cursor_pile, piles)) {
              clear();
            } else {
              mvprintw(0, 0, "no more redeals");
            }
          }
        }
        break;
      case 'm':
        if (selection && cursor_pile) {
          if (legal_move_stack(cursor_pile, selection, selection_pile)) {
            clear();
            selection = NULL;
            selection_pile = NULL;
          }
        }
        break;
      case 'a':
        if (auto_move_to_foundation(piles)) {
          clear();
        }
        break;
      case 10:
        if (cursor_card && !(cursor_card->suit & BOTTOM)) {
          if (move_to_foundation(cursor_card, cursor_pile, piles) || move_to_free_cell(cursor_card, cursor_pile, piles)) {
            clear();
          }
        }
        break;
      case 27:
        clear();
        selection = NULL;
        selection_pile = NULL;
        break;
      case 'r':
        return 1;
      case 'q':
        return 0;
      default:
        mvprintw(0, 0, "%d", ch);
    }
  }
  return 0;
}

void ui_main(Game *game, Theme *theme, int enable_color, unsigned int seed) {
  setlocale(LC_ALL, "");
  initscr();
  if (enable_color) {
    start_color();
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

  while (1) {
    srand(seed);

    Card *deck = new_deck();
    move_stack(deck, shuffle_stack(take_stack(deck->next)));

    Pile *piles = deal_cards(game, deck);

    int redeal = ui_loop(game, theme, piles);
    delete_piles(piles);
    delete_stack(deck);

    if (redeal) {
      seed = time(NULL);
    } else {
      break;
    }
  }

  endwin();
}
