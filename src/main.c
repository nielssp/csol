/* csol
 * Copyright (c) 2017 Niels Sonnich Poulsen (http://nielssp.dk)
 * Licensed under the MIT license.
 * See the LICENSE file or http://opensource.org/licenses/MIT for more information.
 */

#define _XOPEN_SOURCE 500

#include "config.h"
#include "rc.h"
#include "theme.h"
#include "game.h"
#include "ui.h"
#include "util.h"
#include "scores.h"

#include <stdio.h>
#include <stdlib.h>
#ifdef USE_GETOPT
#include <getopt.h>
#else
#include <unistd.h>
#endif
#include <string.h>
#include <errno.h>
#include <time.h>

const char *short_options = "?hvlt:Tms:c:CS";

#ifdef USE_GETOPT
const struct option long_options[] = {
  {"help", no_argument, NULL, 'h'},
  {"version", no_argument, NULL, 'v'},
  {"list", no_argument, NULL, 'l'},
  {"theme", required_argument, NULL, 't'},
  {"themes", no_argument, NULL, 'T'},
  {"mono", no_argument, NULL, 'm'},
  {"seed", required_argument, NULL, 's'},
  {"config", required_argument, NULL, 'c'},
  {"colors", no_argument, NULL, 'C'},
  {"scores", no_argument, NULL, 'S'},
  {0, 0, 0, 0}
};
#endif

enum action { PLAY, LIST_GAMES, LIST_THEMES, LIST_COLORS, SHOW_SCORES };

void describe_option(const char *short_option, const char *long_option, const char *description) {
#ifdef USE_GETOPT
  printf("  -%-14s --%-18s %s\n", short_option, long_option, description);
#else
  printf("  -%-14s %s\n", short_option, description);
#endif
}

char *find_csolrc() {
  FILE *f;
#ifdef USE_XDG_PATHS
  char *config_dir = getenv("XDG_CONFIG_HOME");
  char *config_file = NULL;
  if (config_dir) {
    config_file = combine_paths(config_dir, "csol/csolrc");
  } else {
    config_dir = getenv("HOME");
    if (config_dir) {
      config_file = combine_paths(config_dir, ".config/csol/csolrc");
    }
  }
  if (config_file) {
    f = fopen(config_file, "r");
    if (f) {
      fclose(f);
      return config_file;
    }
    free(config_file);
  }
  config_dir = getenv("XDG_CONFIG_DIRS");
  if (config_dir) {
    int i = 0;
    while (1) {
      if (!config_dir[i] || config_dir[i] == ':') {
        char *dir = malloc(i + 1);
        strncpy(dir, config_dir, i);
        dir[i] = '\0';
        config_file = combine_paths(dir, "csol/csolrc");
        f = fopen(config_file, "r");
        free(dir);
        if (f) {
          fclose(f);
          return config_file;
        }
        free(config_file);
        if (!config_dir[i]) {
          break;
        }
        config_dir = config_dir + i + 1;
        i = 0;
      } else {
        i++;
      }
    }
  } else {
    f = fopen("/etc/xdg/csol/csolrc", "r");
    if (f) {
      fclose(f);
      return strdup("/etc/xdg/csol/csolrc");
    }
  }
#endif
  f = fopen("csolrc", "r");
  if (f) {
    fclose(f);
    return strdup("csolrc");
  }
  return NULL;
}

int main(int argc, char *argv[]) {
  int opt, rc_opt, error;
#ifdef USE_GETOPT
  int option_index = 0;
#endif
  int colors = 1;
  unsigned int seed = time(NULL);
  enum action action = PLAY;
  char *rc_file = NULL;
  char *game_name = NULL;
  char *theme_name = NULL;
  Theme *theme;
  Game *game;
  while ((opt = 
#ifdef USE_GETOPT
        getopt_long(argc, argv, short_options, long_options, &option_index)
#else
        getopt(argc, argv, short_options)
#endif
        ) != -1) {
    switch (opt) {
      case '?':
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
        describe_option("c <file>", "config <file>", "Select configuration file.");
        describe_option("C", "colors", "List colors");
        describe_option("S", "scores", "List scores");
        return 0;
      case 'v':
        puts("csol " CSOL_VERSION);
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
      case 'c':
        rc_file = optarg;
        break;
      case 'C':
        action = LIST_COLORS;
        break;
      case 'S':
        action = SHOW_SCORES;
        break;

    }
  }
  if (optind < argc) {
    game_name = argv[optind];
  }
  rc_opt = 1;
  error = 0;
  if (!rc_file) {
    rc_opt = 0;
    rc_file = find_csolrc();
    if (!rc_file) {
      printf("csolrc: %s\n", strerror(errno));
      error = 1;
    }
  }
  if (!error) {
    printf("Using configuration file: %s\n", rc_file);
    error = !execute_file(rc_file);
    if (!rc_opt) {
      free(rc_file);
    }
  }
  if (!touch_scores_file(argv[0])) {
    error = 1;
  }
  if (!touch_stats_file(argv[0])) {
    error = 1;
  }
  if (error) {
    printf("Configuration errors detected, press enter to continue\n");
    getchar();
  }
  switch (action) {
    case LIST_GAMES: {
      GameList *list;
      load_game_dirs();
      for (list = list_games(); list; list = list->next) {
        printf("%s - %s\n", list->game->name, list->game->title);
      }
      break;
    }
    case LIST_THEMES: {
      ThemeList *list;
      load_theme_dirs();
      for (list = list_themes(); list; list = list->next) {
        printf("%s - %s\n", list->theme->name, list->theme->title);
      }
      break;
    }
    case LIST_COLORS:
      ui_list_colors();
      break;
    case SHOW_SCORES:
      if (game_name) {
        char date[100];
        Score score;
        FILE *f;
        time_t now = time(NULL);
        int date_width = strftime(date, sizeof(date), "%x %X", localtime(&now));
        printf("%-*s %-3s %10s %10s\n", date_width, "Date", "Won", "Time", "Score");
        if (!scores_file_path) {
          printf("No scores file\n");
          break;
        }
        f = fopen(scores_file_path, "rb");
        if (!f) {
          printf("%s: %s\n", scores_file_path, strerror(errno));
          break;
        }
        while (read_scores(f, &score)) {
          if (strcmp(score.game, game_name) == 0) {
            if (strftime(date, sizeof(date), "%x %X", localtime(&score.timestamp))) {
              char time[18];
              format_time(time, score.duration);
              printf("%-*s %-3s %10s %10d\n", date_width, date, score.victory ? "X" : "", time, score.score);
            } else {
              printf("strftime() failed: %s\n", strerror(errno));
            }
          }
          free(score.game);
        }
        fclose(f);
      } else {
        Stats *stats = get_stats();
        Stats *current;
        int max = 4;
        for (current = stats; current; current = current->next) {
          int l = strlen(current->game);
          if (l > max) {
            max = l;
          }
        }
        printf("%-*s %7s %7s %4s %13s %10s %10s\n", max, "Game", "Won", "Lost", "%", "Time spent", "Best time", "Best score");
        for (current = stats; current; current = current->next) {
          char total_time_played[18], best_time[18];
          format_time(total_time_played, current->total_time_played);
          format_time(best_time, current->best_time);
          printf("%-*s %7d %7d %3d%% %13s %10s %10d\n", max, current->game, current->times_won, current->times_played - current->times_won, current->times_won * 100 / current->times_played, total_time_played, best_time, current->best_score);
        }
        delete_stats(stats);
      }
      break;
    case PLAY:
      if (theme_name == NULL) {
        theme_name = get_property("default_theme");
        if (theme_name == NULL) {
          printf("default_theme not set\n");
          return 1;
        }
      }
      theme = get_theme(theme_name);
      if (!theme) {
        printf("theme not found: '%s'\n", theme_name);
        return 1;
      }
      if (game_name == NULL) {
        game_name = get_property("default_game");
        if (game_name == NULL) {
          printf("default_game not set\n");
          return 1;
        }
      }
      game = get_game(game_name);
      if (!game) {
        printf("game not found: '%s'\n", game_name);
        return 1;
      }
      ui_main(game, theme, colors, seed);
      break;
  }
  return 0;
}
