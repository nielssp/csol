#include <stdlib.h>

#include "game.h"

GameList *first_game = NULL;
GameList *last_game = NULL;

Game *new_game() {
  return malloc(sizeof(Game));
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

