#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <time.h>

#define COLORS

#define S_HEART "♥"
#define S_SPADE "♠"
#define S_DIAMOND "♦"
#define S_CLUB "♣"

#define EMPTY_STACK_TOP "┌────┐"
#define EMPTY_STACK_MID "│    │"
#define EMPTY_STACK_BOT "└────┘"
#define CARD_FRONT_TOP  "┌────┐"
#define CARD_FRONT_MID  "│    │"
#define CARD_FRONT_BOT  "└────┘"
#define CARD_BACK_TOP   "┌────┐"
#define CARD_BACK_MID   "│    │"
#define CARD_BACK_BOT   "└────┘"

#define CARD_HEIGHT 4

#define HEART 2
#define DIAMOND 10
#define SPADE 4
#define CLUB 12

#define TABLEAU 9
#define FOUNDATION 17

#define BOTTOM 1
#define RED 2
#define BLACK 4

#define ACE 1
#define JACK 11
#define QUEEN 12
#define KING 13

typedef struct game_rule GameRule;

#define RULE_TABLEAU 1
#define RULE_STOCK 2
#define RULE_FOUNDATION 3
#define RULE_WASTE 4

struct game_rule {
  int rule;
  int y;
  int x;
};

const char SUITS[] = {HEART, DIAMOND, SPADE, CLUB};

typedef struct card Card;

struct card {
  Card *prev;
  Card *next;
  char up;
  char suit;
  char rank;
};

int cur_x = 0;
int cur_y = 0;

int max_x = 0;
int max_y = 0;

Card *selection = NULL;
Card *under_cursor = NULL;

Card *new_card(char suit, char rank) {
  Card *card = malloc(sizeof(Card));
  card->prev = NULL;
  card->next = NULL;
  card->up = 1;
  card->suit = suit;
  card->rank = rank;
  return card;
}

Card *new_deck() {
  Card *deck = new_card(BOTTOM, 0);
  Card *prev = deck;
  for (char suit = 0; suit < 4; suit++) {
    for (char rank = 1; rank <= 13; rank++) {
      Card *card = new_card(SUITS[suit], rank);
      prev->next = card;
      card->prev = prev;
      prev = card;
    }
  }
  return deck;
}

char *card_suit(Card *card) {
  switch (card->suit) {
    case HEART:
      return S_HEART;
    case SPADE:
      return S_SPADE;
    case DIAMOND:
      return S_DIAMOND;
    case CLUB:
      return S_CLUB;
  }
  return "";
}

void print_card_name_l(int y, int x, Card *card) {
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
  printw(card_suit(card));
}

void print_card_name_r(int y, int x, Card *card) {
  mvprintw(y, x - 1 - (card->rank == 10), card_suit(card));
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


Card *shuffle_stack(Card *stack) {
  int n = 2;
  if (!stack->next) {
    return stack;
  }
  Card *new = stack;
  Card *next = stack->next;
  new->next = NULL;
  new->prev = NULL;
  while (next) {
    Card *next_next = next->next;
    int i = rand() % n;
    if (i == 0) {
      new->prev = next;
      next->prev = NULL;
      next->next = new;
      new = next;
    } else {
      Card *c = new;
      for (int j = 1; j < i; j++) {
        c = c->next;
      }
      if (c->next) {
        c->next->prev = next;
      }
      next->next = c->next;
      c->next = next;
      next->prev = c;
    }
    n++;
    next = next_next;
  }
  return new;
}

Card *take_card(Card *card) {
  if (card->prev) {
    card->prev->next = card->next;
  }
  if (card->next) {
    card->next->prev = card->prev;
  }
  card->prev = NULL;
  card->next = NULL;
  return card;
}

Card *take_stack(Card *stack) {
  if (stack->prev) {
    stack->prev->next = NULL;
  }
  stack->prev = NULL;
  return stack;
}

void move_stack(Card *dest, Card *src) {
  if (dest->next)
    return move_stack(dest->next, src);
  dest->next = src;
  if (src->prev) src->prev->next = NULL;
  src->prev = dest;
}

Card *get_bottom(Card *stack) {
  if (stack->prev)
    return get_bottom(stack->prev);
  return stack;
}

char get_stack_type(Card *stack) {
  return get_bottom(stack)->suit;
}

int move_to_tableau(Card *dest, Card *src) {
  if (dest->next)
    return move_to_tableau(dest->next, src);
  if (dest->suit & BOTTOM && src->rank == KING || ((dest->suit & RED) != (src->suit & RED) && src->rank == dest->rank - 1)) {
    dest->next = src;
    if (src->prev) src->prev->next = NULL;
    src->prev = dest;
    return 1;
  }
  return 0;
}

int move_to_foundation(Card *dest, Card *src) {
  if (src->next)
    return 0;
  if (dest->next)
    return move_to_foundation(dest->next, src);
  if (dest->suit & BOTTOM && src->rank == ACE || (dest->suit == src->suit && src->rank == dest->rank + 1)) {
    dest->next = src;
    if (src->prev) src->prev->next = NULL;
    src->prev = dest;
    return 1;
  }
  return 0;
}

int legal_move_stack(Card *dest, Card *src) {
  if (get_bottom(src) == get_bottom(dest)) {
    return 0;
  }
  switch (get_stack_type(dest)) {
    case TABLEAU:
      return move_to_tableau(dest, src);
    case FOUNDATION:
      return move_to_foundation(dest, src);
  }
  return 0;
}

void print_card(int y, int x, Card *card, int full) {
  int y2 = y + full * (CARD_HEIGHT - 1);
  if (y2 > max_y) max_y = y2;
  if (x > max_x) max_x = x;
  if (y <= cur_y && y2 >= cur_y && x == cur_x) under_cursor = card;
  y = 1 + y;
  x = 2 + x * 8;
  if (card == selection) {
    attron(A_REVERSE);
  }
  if (card->suit & BOTTOM) {
    attron(COLOR_PAIR(1));
    mvprintw(y, x, EMPTY_STACK_TOP);
    if (full && CARD_HEIGHT > 1) {
      for (int i = 1; i < CARD_HEIGHT - 1; i++) {
        mvprintw(y + i, x, EMPTY_STACK_MID);
      }
      mvprintw(y + CARD_HEIGHT - 1, x, EMPTY_STACK_BOT);
    }
    if (card->rank > 0) {
      print_card_name_l(y, x + 1, card);
    }
  } else if (!card->up) {
    attron(COLOR_PAIR(2));
    mvprintw(y, x, CARD_BACK_TOP);
    if (full && CARD_HEIGHT > 1) {
      for (int i = 1; i < CARD_HEIGHT - 1; i++) {
        mvprintw(y + i, x, CARD_BACK_MID);
      }
      mvprintw(y + CARD_HEIGHT - 1, x, CARD_BACK_BOT);
    }
  } else {
    if (card->suit & RED) {
      attron(COLOR_PAIR(3));
    } else {
      attron(COLOR_PAIR(4));
    }
    mvprintw(y, x, CARD_FRONT_TOP);
    if (full && CARD_HEIGHT > 1) {
      for (int i = 1; i < CARD_HEIGHT - 1; i++) {
        mvprintw(y + i, x, CARD_FRONT_MID);
      }
      mvprintw(y + CARD_HEIGHT - 1, x, CARD_FRONT_BOT);
      print_card_name_r(y + CARD_HEIGHT - 1, x + 4, card);
    }
    print_card_name_l(y, x + 1, card);
  }
  attroff(A_REVERSE);
}

void print_Cardop(int y, int x, Card *card) {
  print_card(y, x, card, 0);
}

void print_card_full(int y, int x, Card *card) {
  print_card(y, x, card, 1);
}

void print_stack(int y, int x, Card *bottom) {
  if (bottom->next) {
    print_stack(y, x, bottom->next);
  } else {
    print_card_full(y, x, bottom);
  }
}

void print_tableau(int y, int x, Card *bottom) {
  if (bottom->next && bottom->suit & BOTTOM) {
    print_tableau(y, x, bottom->next);
  } else {
    if (bottom->next) {
      print_Cardop(y, x, bottom);
      print_tableau(y + 1, x, bottom->next);
    } else {
      print_card_full(y, x, bottom);
    }
  }
}

int main() {
  int ch, run = 1;
  setlocale(LC_ALL, "");
  srand(time(NULL));
  initscr();
#ifdef COLORS
  start_color();
//  init_pair(1, 8, 8);
  init_pair(1, 7, 0);
  init_pair(2, 7, 4);
  init_pair(3, 1, 7);
  init_pair(4, 0, 7);
#endif
  raw();
  clear();
  curs_set(1);
  keypad(stdscr, 1);
  noecho();

  Card *deck = new_deck();
  move_stack(deck, shuffle_stack(take_stack(deck->next)));

  Card *foundations[4] = { NULL };
  for (int i = 0; i < 4; i++) {
    foundations[i] = new_card(FOUNDATION, ACE);
  }

  Card *tableaus[7] = { NULL };
  for (int i = 0; i < 7; i++) {
    tableaus[i] = new_card(TABLEAU, KING);
  }

  move_stack(tableaus[0], take_card(deck->next));

  for (int i = 1; i < 7; i++) {
    for (int j = 0; j < i; j++) {
      Card *card = take_card(deck->next);
      card->up = 0;
      move_stack(tableaus[i], card);
    }
  }
  for (int i = 1; i < 7; i++) {
    for (int j = 0; j < 5; j++) {
      move_stack(tableaus[i], take_card(deck->next));
    }
  }


  while (run) {
    under_cursor = NULL;
    max_x = max_y = 0;
    for (int i = 0; i < 4; i++) {
      print_stack(0, 3 + i, foundations[i]);
    }
    for (int i = 0; i < 7; i++) {
      print_tableau(1 + CARD_HEIGHT, i, tableaus[i]);
    }
    move(1 + cur_y, 2 + cur_x * 8);

    refresh();

    ch = getch();

    switch (ch) {
      case 'h':
      case 260:
        cur_x--;
        if (cur_x < 0) cur_x = 0;
        break;
      case 'j':
      case 258:
        cur_y++;
        if (cur_y > max_y) cur_y = max_y;
        break;
      case 'k':
      case 259:
        cur_y--;
        if (cur_y < 0) cur_y = 0;
        break;
      case 'l':
      case 261:
        cur_x++;
        if (cur_x > max_x) cur_x = max_x;
        break;
      case 'm':
        if (selection && under_cursor && !(selection->suit & BOTTOM)) {
          if (legal_move_stack(under_cursor, selection)) {
            clear();
            selection = NULL;
          }
        }
        break;
      case 10:
      case 'f':
        if (under_cursor && under_cursor->up && !under_cursor->next) {
          for (int i = 0; i < 4; i++) {
            if (legal_move_stack(foundations[i], under_cursor)) {
              clear();
              selection = NULL;
              break;
            }
          }
        }
        break;
      case 'a':
        for (int i = 0; i < 7; i++) {
          Card *c = tableaus[i];
          while (c->next) c = c->next;
          if (!(c->suit & BOTTOM)) {
            if (!c->up) {
              c->up = 1;
              break;
            } else {
              int legal = 0;
              for (int j = 0; j < 4; j++) {
                if (legal_move_stack(foundations[j], c)) {
                  clear();
                  legal = 1;
                  break;
                }
              }
              if (legal) {
                break;
              }
            }
          }
        }
        break;
      case ' ':
        if (under_cursor) {
          if (!under_cursor->up) {
            if (!under_cursor->next) {
              under_cursor->up = 1;
            }
          } else if (!(under_cursor->suit & BOTTOM)) {
            if (selection == under_cursor && !under_cursor->next) {
              for (int i = 0; i < 4; i++) {
                if (legal_move_stack(foundations[i], under_cursor)) {
                  clear();
                  selection = NULL;
                  break;
                }
              }
              if (!selection) {
                break;
              }
            }
            selection = under_cursor;
          }
        }
        break;
      case 27:
        clear();
        selection = NULL;
        break;
      case 'q':
        run = 0;
        break;
      default:
        mvprintw(0, 0, "%d", ch);
    }
  }
  endwin();
  return 0;
}
