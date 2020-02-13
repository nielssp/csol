/* csol
 * Copyright (c) 2020 Niels Sonnich Poulsen (http://nielssp.dk)
 * Licensed under the MIT license.
 * See the LICENSE file or http://opensource.org/licenses/MIT for more information.
 */

#include "scores.h"

#include "util.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

struct stats {
  struct stats *next;
  char *game;
  int times_played;
  int times_won;
  int total_time_played;
  int best_time;
  int best_score;
  time_t first_played;
  time_t last_played;
};

int scores_enabled = 0;
char *scores_file_path = NULL;

int stats_enabled = 0;
char *stats_file_path = NULL;

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

void update_stats(const char *game_name, int victory, int score, int duration, char *date) {
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
        int times_played, times_won, total_time_played, best_time, best_score;
        char first_played[26];
        long offset = ftell(f);
        if (fscanf(f, "%d,%d,%d,%d,%d,%25s", &times_played, &times_won, &total_time_played,
              &best_time, &best_score, first_played)) {
          times_played++;
          times_won += victory;
          total_time_played += duration;
          if (victory && (best_time < 0 || duration < best_time)) best_time = duration;
          if (victory && (best_score < 0 || score < best_score)) best_score = score;
          fseek(f, offset, SEEK_SET);
          fprintf(f, "%11d,%11d,%11d,%11d,%11d,%s,%s\n", times_played, times_won, total_time_played,
              best_time, best_score, first_played, date);
          found = 1;
          break;
        }
      }
    }
    fscanf(f, "%*[^\n]\n");
  }
  if (!found) {
    fseek(f, 0, SEEK_END);
    fprintf(f, "%s,%11d,%11d,%11d,%11d,%11d,%s,%s\n", game_name, 1, victory, duration,
        victory ? duration : -1, victory ? score : -1, date, date);
  }
  fclose(f);
}

int append_score(const char *game_name, int victory, int score, time_t start_time) {
  FILE *f;
  struct tm *utc;
  time_t now;
  char date[26];
  int duration;
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
  duration = now - start_time;
  if (utc) {
    if (strftime(date, sizeof(date), "%Y-%m-%dT%H:%M:%SZ", utc)) {
      fprintf(f, "%s,%s,%d,%d,%d\n", date, game_name, victory, score, duration);
      update_stats(game_name, victory, score, duration, date);
    } else {
      printf("Saving score failed: %s\n", strerror(errno));
    }
  } else {
    printf("Saving score failed: %s\n", strerror(errno));
  }
  fclose(f);
  return 1;
}
