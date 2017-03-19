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

const char SUITS[] = {HEART, DIAMOND, SPADE, CLUB};

typedef struct card Card;

Card *new_card(char suit, char rank);
Card *new_deck();
Card *shuffle_stack(Card *stack);
Card *take_card(Card *card);
Card *take_stack(Card *stack);
void move_stack(Card *dest, Card *src);
Card *get_bottom(Card *stack);
char get_stack_type(Card *stack);

#endif
