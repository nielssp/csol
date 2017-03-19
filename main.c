#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include "rc.h"
#include "theme.h"
#include "game.h"

const char *short_options = "hvlt:Tms:";

const struct option long_options[] = {
  {"help", no_argument, NULL, 'h'},
  {"version", no_argument, NULL, 'v'},
  {"list", no_argument, NULL, 'l'},
  {"theme", required_argument, NULL, 't'},
  {"themes", no_argument, NULL, 'T'},
  {"mono", no_argument, NULL, 'm'},
  {"seed", required_argument, NULL, 's'},
  {0, 0, 0, 0}
};

enum action { PLAY, LIST_GAMES, LIST_THEMES };

void describe_option(const char *short_option, const char *long_option, const char *description) {
  printf("  -%-14s --%-18s %s\n", short_option, long_option, description);
}

int main(int argc, char *argv[]) {
  int opt;
  int option_index = 0;
  int colors = 1;
  unsigned int seed = time(NULL);
  enum action action = PLAY;
  char *game_name = "yukon";
  char *theme_name = "default";
  while ((opt = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
    switch (opt) {
      case 'h':
        printf("usage: %s [options] [game]\n", argv[0]);
        puts("options:");
        describe_option("h", "help", "Show help.");
        describe_option("v", "version", "Show version information.");
        describe_option("l", "list", "List available games.");
        describe_option("t <name>", "theme <name>", "Select a theme.");
        describe_option("T", "themes", "List available themes.");
        describe_option("m", "mono", "Disable colors.");
        describe_option("s <seed>", "seed <seed>", "Select seed.");
        return 0;
      case 'v':
        puts("csol 0.1.0");
        return 0;
      case 'l':
        action = LIST_GAMES;
        break;
      case 't':
        theme_name = optarg;
        break;
      case 'T':
        action = LIST_THEMES;
        break;
      case 'm':
        colors = 0;
        break;
      case 's':
        seed = atol(optarg);
        break;
    }
  }
  if (optind < argc) {
    game_name = argv[optind];
  }
  FILE *f = fopen("csolrc", "r");
  if (!f) {
    printf("csolrc: %s\n", strerror(errno));
  } else {
    execute_file(f);
    fclose(f);
  }
  Theme *theme;
  Game *game;
  switch (action) {
    case LIST_GAMES:
      for (GameList *list = list_games(); list; list = list->next) {
        printf("%s - %s\n", list->game->name, list->game->title);
      }
      break;
    case LIST_THEMES:
      for (ThemeList *list = list_themes(); list; list = list->next) {
        printf("%s - %s\n", list->theme->name, list->theme->title);
      }
      break;
    case PLAY:
      theme = get_theme(theme_name);
      if (!theme) {
        printf("theme not found: %s\n", theme_name);
        return 1;
      }
      game = get_game(game_name);
      if (!game) {
        printf("game not found: %s\n", game_name);
        return 1;
      }
      break;
  }
  return 0;
}
