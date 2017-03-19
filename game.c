#include <stdlib.h>
#include <string.h>

#include "game.h"
#include "card.h"

GameList *first_game = NULL;
GameList *last_game = NULL;

Game *new_game() {
  Game *game = malloc(sizeof(Game));
  game->name = NULL;
  game->title = NULL;
  game->first_rule = NULL;
  game->last_rule = NULL;
  return game;
}

GameRule *new_game_rule(GameRuleType type) {
  GameRule *rule = malloc(sizeof(GameRule));
  rule->next = NULL;
  rule->type = type;
  rule->x = 0;
  rule->y = 0;
  rule->deal = 0;
  rule->hide = 0;
  rule->first_suit = SUIT_ANY;
  rule->first_rank = RANK_ANY;
  rule->next_suit = SUIT_ANY;
  rule->next_rank = RANK_ANY;
  rule->move_group = MOVE_NONE;
  switch (type) {
    case RULE_FOUNDATION:
      rule->first_rank = RANK_ACE;
      rule->next_suit = SUIT_SAME;
      rule->next_rank = RANK_UP;
      break;
    case RULE_TABLEAU:
      rule->next_suit = SUIT_ANY;
      rule->next_rank = RANK_DOWN;
      break;
    case RULE_STOCK:
      rule->first_suit = SUIT_NONE;
      rule->first_rank = RANK_NONE;
      rule->next_suit = SUIT_NONE;
      rule->next_rank = RANK_NONE;
      break;
    case RULE_WASTE:
      // TODO: only from stock
      break;
  }
  return rule;
}

void register_game(Game *game) {
  if (game->name) {
    GameList *next = malloc(sizeof(GameList));
    next->game = game;
    next->next = NULL;
    if (last_game) {
      last_game->next = next;
      last_game = next;
    } else {
      first_game = last_game = next;
    }
  }
}

GameList *list_games() {
  return first_game;
}

Game *get_game(const char *name) {
  for (GameList *games = list_games(); games; games = games->next) {
    if (strcmp(games->game->name, name) == 0) {
      return games->game;
    }
  }
  return NULL;
}

