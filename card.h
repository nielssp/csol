/* yuk
 * Copyright (c) 2017 Niels Sonnich Poulsen (http://nielssp.dk)
 * Licensed under the MIT license.
 * See the LICENSE file or http://opensource.org/licenses/MIT for more information.
 */

#ifndef CARD_H
#define CARD_H

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

extern char suits[];

typedef struct card Card;

struct card {
  Card *prev;
  Card *next;
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
