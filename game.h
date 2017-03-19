#ifndef GAME_H
#define GAME_H

typedef struct game_list GameList;
typedef struct game Game;
typedef struct game_rule GameRule;
typedef enum {
  RULE_TABLEAU,
  RULE_STOCK,
  RULE_FOUNDATION,
  RULE_WASTE
} GameRuleType;

struct game_list {
  Game *game;
  GameList *next;
};

struct game {
  char *name;
  char *title;
  GameRule *rules;
};

struct game_rule {
  GameRule* next;
  GameRuleType type;
  short x;
  short y;
  short deal;
  short hide;
  char first_suit;
  char first_rank;
  char next_suit;
  char next_rank;
  char move_group;
};

Game *new_game();
void register_game(Game *game);
GameList *list_games();

#endif
