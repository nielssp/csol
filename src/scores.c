/* csol
 * Copyright (c) 2020 Niels Sonnich Poulsen (http://nielssp.dk)
 * Licensed under the MIT license.
 * See the LICENSE file or http://opensource.org/licenses/MIT for more information.
 */

#include "scores.h"

#include "util.h"
#include "csv.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

int scores_enabled = 0;
char *scores_file_path = NULL;

int stats_enabled = 0;
char *stats_file_path = NULL;

int show_score = 0;

int touch_scores_file(const char *arg0) {
  FILE *f;
  if (!scores_enabled) {
    return 1;
  }
  if (!scores_file_path) {
    scores_file_path = find_data_file("scores.csv", arg0);
  }
  if (!scores_file_path) {
    printf("Could not find a place to put scores.csv\n");
    return 0;
  }
  f = fopen(scores_file_path, "r+");
  if (!f) {
    f = fopen(scores_file_path, "w");
    if (!f) {
      printf("%s: %s\n", scores_file_path, strerror(errno));
      return 0;
    }
  }
  fclose(f);
  return 1;
}

int touch_stats_file(const char *arg0) {
  FILE *f;
  if (!stats_enabled) {
    return 1;
  }
  if (!stats_file_path) {
    stats_file_path = find_data_file("stats.csv", arg0);
  }
  if (!stats_file_path) {
    printf("Could not find a place to put stats.csv\n");
    return 0;
  }
  f = fopen(stats_file_path, "r+");
  if (!f) {
    f = fopen(stats_file_path, "w");
    if (!f) {
      printf("%s: %s\n", scores_file_path, strerror(errno));
      return 0;
    }
  }
  fclose(f);
  return 1;
}

static void update_stats(const char *game_name, int victory, int score,
    int duration, char *date, Stats *stats_out) {
  FILE *f;
  int found = 0;
  if (!stats_file_path) {
    return;
  }
  f = fopen(stats_file_path, "rb+");
  if (!f) {
    printf("%s: %s\n", stats_file_path, strerror(errno));
    return;
  }
  while (!feof(f)) {
    char game_name_buffer[100];
    if (fscanf(f, "%99[^,],", game_name_buffer)) {
      if (strcmp(game_name_buffer, game_name) == 0) {
        Stats stats;
        char first_played[26];
        long offset = ftell(f);
        if (fscanf(f, "%d,%d,%d,%d,%d,%25s", &stats.times_played,
              &stats.times_won, &stats.total_time_played, &stats.best_time,
              &stats.best_score, first_played)) {
          stats.times_played++;
          stats.times_won += victory;
          stats.total_time_played += duration;
          if (victory && (stats.best_time < 0 || duration < stats.best_time)) {
            stats.best_time = duration;
          }
          if (victory && score > stats.best_score) {
            stats.best_score = score;
          }
          fseek(f, offset, SEEK_SET);
          fprintf(f, "%11d,%11d,%11d,%11d,%11d,%s,%s\n", stats.times_played,
              stats.times_won, stats.total_time_played, stats.best_time,
              stats.best_score, first_played, date);
          if (stats_out) {
            *stats_out = stats;
          }
          found = 1;
          break;
        }
      }
    }
    fscanf(f, "%*[^\n]\n");
  }
  if (!found) {
    fseek(f, 0, SEEK_END);
    fprintf(f, "%s,%11d,%11d,%11d,%11d,%11d,%s,%s\n", game_name, 1, victory,
        duration, victory ? duration : -1, victory ? score : -1, date, date);
    if (stats_out) {
      stats_out->times_played = 1;
      stats_out->times_won = victory;
      stats_out->total_time_played = duration;
      stats_out->best_time = victory ? duration : -1;
      stats_out->best_score = victory ? score : -1;
    }
  }
  fclose(f);
}

int append_score(const char *game_name, int victory, int score,
    int duration, Stats *stats_out) {
  FILE *f;
  struct tm *utc;
  time_t now;
  char date[26];
  if (stats_out) {
    stats_out->best_time = -1;
    stats_out->best_score = -1;
  }
  if (!scores_file_path) {
    return 1;
  }
  f = fopen(scores_file_path, "a");
  if (!f) {
    printf("%s: %s\n", scores_file_path, strerror(errno));
    return 0;
  }
  now = time(NULL);
  utc = gmtime(&now);
  if (utc) {
    if (strftime(date, sizeof(date), "%Y-%m-%dT%H:%M:%SZ", utc)) {
      fprintf(f, "%s,%s,%d,%d,%d\n", date, game_name, victory, score, duration);
      update_stats(game_name, victory, score, duration, date, stats_out);
    } else {
      printf("Saving score failed: %s\n", strerror(errno));
    }
  } else {
    printf("Saving score failed: %s\n", strerror(errno));
  }
  fclose(f);
  return 1;
}

Stats *get_stats() {
  Stats *stats = NULL, *head;
  FILE *f;
  if (!stats_file_path) {
    return NULL;
  }
  f = fopen(stats_file_path, "rb");
  if (!f) {
    printf("%s: %s\n", stats_file_path, strerror(errno));
    return NULL;
  }
  head = malloc(sizeof(Stats));
  while (read_csv(f, "siiiiitt", &head->game, &head->times_played,
        &head->times_won, &head->total_time_played, &head->best_time,
        &head->best_score, &head->first_played, &head->last_played)) {
    head->next = stats;
    stats = head;
    head = malloc(sizeof(Stats));
  }
  free(head);
  fclose(f);
  return stats;
}

void delete_stats(Stats *stats) {
  if (stats->next) {
    delete_stats(stats->next);
  }
  if (stats->game) {
    free(stats->game);
  }
  free(stats);
}

int read_scores(FILE *f, Score *score) {
  return read_csv(f, "tsiii", &score->timestamp, &score->game, &score->victory, &score->score, &score->duration);
}
