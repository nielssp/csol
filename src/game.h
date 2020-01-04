/* csol
 * Copyright (c) 2017 Niels Sonnich Poulsen (http://nielssp.dk)
 * Licensed under the MIT license.
 * See the LICENSE file or http://opensource.org/licenses/MIT for more information.
 */

#ifndef GAME_H
#define GAME_H

#include "card.h"

typedef struct pile Pile;
typedef struct game_list GameList;
typedef struct game Game;
typedef struct game_rule GameRule;
typedef enum {
  RULE_ANY,
  RULE_TABLEAU,
  RULE_STOCK,
  RULE_FOUNDATION,
  RULE_CELL,
  RULE_WASTE
} GameRuleType;
typedef enum {
  SUIT_NONE,
  SUIT_ANY,
  SUIT_HEART,
  SUIT_DIAMOND,
  SUIT_SPADE,
  SUIT_CLUB,
  SUIT_RED,
  SUIT_BLACK,
  SUIT_SAME,
  SUIT_SAME_COLOR,
  SUIT_DIFF,
  SUIT_DIFF_COLOR
} GameRuleSuit;
typedef enum {
  RANK_NONE,
  RANK_ACE,
  RANK_2,
  RANK_3,
  RANK_4,
  RANK_5,
  RANK_6,
  RANK_7,
  RANK_8,
  RANK_9,
  RANK_10,
  RANK_JACK,
  RANK_QUEEN,
  RANK_KING,
  RANK_ANY,
  RANK_SAME,
  RANK_DOWN,
  RANK_UP,
  RANK_UP_DOWN,
  RANK_LOWER,
  RANK_HIGHER
} GameRuleRank;
typedef enum {
  MOVE_GROUP,
  MOVE_ANY,
  MOVE_ONE
} GameRuleMove;

struct game_list {
  Game *game;
  GameList *next;
};

struct game {
  char *name;
  char *title;
  GameRule *first_rule;
  GameRule *last_rule;
};

struct game_rule {
  GameRule* next;
  GameRuleType type;
  short x;
  short y;
  short deal;
  short redeals;
  short hide;
  GameRuleSuit first_suit;
  GameRuleRank first_rank;
  GameRuleSuit next_suit;
  GameRuleRank next_rank;
  GameRuleMove move_group;
  GameRuleType from;
  GameRuleRank win_rank;
};

struct pile {
  Pile *next;
  Card *stack;
  GameRule *rule;
  int redeals;
};

extern int move_counter;

Game *new_game();
GameRule *new_game_rule(GameRuleType type);
void register_game(Game *game);
void register_game_dir(const char *cwd, const char *dir);
void load_game_dirs();
GameList *list_games();
Game *get_game(const char *name);
Pile *deal_cards(Game *game, Card *deck);
void delete_piles(Pile *piles);
int legal_move_stack(Pile *dest, Card *src, Pile *src_pile, Pile *pile);
int move_to_waste(Card *card, Pile *stock, Pile *piles);
int redeal(Pile *stock, Pile *piles);
int move_to_foundation(Card *src, Pile *src_pile, Pile *piles);
int move_to_free_cell(Card *src, Pile *src_pile, Pile *piles);
int auto_move_to_foundation(Pile *piles);
int turn_card(Card *card);
int check_win_condition(Pile *piles);

char *get_move_error();
void clear_undo_history();
void undo_move();
void redo_move();

#endif
