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
  RANK_ANY,
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
  MOVE_NONE
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
  short hide;
  GameRuleSuit first_suit;
  GameRuleRank first_rank;
  GameRuleSuit next_suit;
  GameRuleRank next_rank;
  GameRuleMove move_group;
};

Game *new_game();
GameRule *new_game_rule(GameRuleType type);
void register_game(Game *game);
GameList *list_games();
Game *get_game(const char *name);

#endif
