/* csol
 * Copyright (c) 2017 Niels Sonnich Poulsen (http://nielssp.dk)
 * Licensed under the MIT license.
 * See the LICENSE file or http://opensource.org/licenses/MIT for more information.
 */

#ifndef CARD_H
#define CARD_H

/* Card suit bit masks:
 *
 * BOTTOM:     00000001
 * TABLEAU:    00001001
 * FOUNDATION: 00010001
 *
 * RED:        00000010
 * HEART:      00000010
 * DIAMOND:    00001010
 *
 * BLACK:      00000100
 * SPADE:      00000100
 * CLUB:       00001100
 */
#define BOTTOM     0x01
#define TABLEAU    0x09
#define FOUNDATION 0x11

#define RED        0x02
#define HEART      0x02
#define DIAMOND    0x0A

#define BLACK      0x04
#define SPADE      0x04
#define CLUB       0x0C

#define ACE    1
#define JACK  11
#define QUEEN 12
#define KING  13

#define IS_BOTTOM(card) ((card)->suit & BOTTOM)
#define NOT_BOTTOM(card) (!((card)->suit & BOTTOM))

extern char suits[];

typedef struct card Card;

struct card {
  Card *prev;
  Card *next;
  int x;
  int y;
  char up;
  char suit;
  char rank;
}; 

Card *new_card(char suit, char rank);
void delete_stack(Card *stack);
Card *new_deck();
Card *shuffle_stack(Card *stack);
Card *take_card(Card *card);
Card *take_stack(Card *stack);
void move_stack(Card *dest, Card *src);
Card *get_bottom(Card *stack);
Card *get_top(Card *stack);
char get_stack_type(Card *stack);

#endif
