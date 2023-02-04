/* csol
 * Copyright (c) 2020 Niels Sonnich Poulsen (http://nielssp.dk)
 * Licensed under the MIT license.
 * See the LICENSE file or http://opensource.org/licenses/MIT for more information.
 */

#define _XOPEN_SOURCE 500

#include "scores.h"

#include "util.h"
#include "csv.h"
#include "error.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <string.h>

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

static void update_stats(const char *game_name, int victory, int32_t score,
    int32_t duration, time_t date, Stats *stats_out) {
  Stats *current, *existing = NULL, *stats = get_stats();
  for (current = stats; current; current = current->next) {
    if (strcmp(current->game, game_name) == 0) {
      existing = current;
      break;
    }
  }
  if (!existing) {
    existing = malloc(sizeof(Stats));
    existing->next = stats;
    existing->game = strdup(game_name);
    existing->times_played = 0;
    existing->times_won = 0;
    existing->total_time_played = 0;
    existing->best_time = -1;
    existing->best_score = -1;
    existing->first_played = date;
    stats = existing;
  }
  existing->times_played++;
  existing->times_won += victory;
  existing->total_time_played += duration;
  if (victory && (existing->best_time < 0 || duration < existing->best_time)) {
    existing->best_time = duration;
  }
  if (victory && score > existing->best_score) {
    existing->best_score = score;
  }
  existing->last_played = date;
  if (stats_out) {
    *stats_out = *existing;
  }
  put_stats(stats);
  delete_stats(stats);
}

int append_score(const char *game_name, int victory, int32_t score,
    int32_t duration, Stats *stats_out) {
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
    print_error("Error: Scores file could not be opened: %s: %s", scores_file_path, strerror(errno));
    return 0;
  }
  now = time(NULL);
  utc = gmtime(&now);
  if (utc) {
    if (strftime(date, sizeof(date), "%Y-%m-%dT%H:%M:%SZ", utc)) {
      fprintf(f, "%s,%s,%d,%" PRId32 ",%" PRId32 "\n", date, game_name, victory, score, duration);
      update_stats(game_name, victory, score, duration, now, stats_out);
    } else {
      print_error("Saving score failed: %s", strerror(errno));
    }
  } else {
    print_error("Saving score failed: %s", strerror(errno));
  }
  fclose(f);
  return 1;
}

static void reverse_stats(Stats **stats) {
  Stats *current = *stats;
  Stats *previous = NULL;
  while (current) {
    Stats *next = current->next;
    current->next = previous;
    previous = current;
    current = next;
  }
  *stats = previous;
}

Stats *get_stats() {
  Stats *stats = NULL, *head;
  FILE *f;
  if (!stats_file_path) {
    return NULL;
  }
  f = fopen(stats_file_path, "rb");
  if (!f) {
    print_error("Error: Stats file could not be opened: %s: %s", stats_file_path, strerror(errno));
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
  reverse_stats(&stats);
  return stats;
}

void put_stats(Stats *stats) {
  Stats *head;
  FILE *f;
  if (!stats_file_path) {
    return;
  }
  f = fopen(stats_file_path, "wb");
  if (!f) {
    print_error("Error: Stats file could not be opened: %s: %s", stats_file_path, strerror(errno));
    return;
  }
  for (head = stats; head; head = head->next) {
    char date[26];
    struct tm *utc;
    fprintf(f, "%s,%" PRId32 ",%" PRId32 ",%" PRId32 ",%" PRId32 ",%" PRId32 ",", head->game,
        head->times_played, head->times_won, head->total_time_played, head->best_time, head->best_score);
    utc = gmtime(&head->first_played);
    if (utc && strftime(date, sizeof(date), "%Y-%m-%dT%H:%M:%SZ", utc)) {
      fprintf(f, "%s", date);
    } else {
      print_error("Saving score failed: %s", strerror(errno));
    }
    fprintf(f, ",");
    utc = gmtime(&head->last_played);
    if (utc && strftime(date, sizeof(date), "%Y-%m-%dT%H:%M:%SZ", utc)) {
      fprintf(f, "%s", date);
    } else {
      print_error("Saving score failed: %s", strerror(errno));
    }
    fprintf(f, "\n");
  }
  fclose(f);
}

void delete_stats(Stats *stats) {
  if (!stats) {
    return;
  }
  delete_stats(stats->next);
  if (stats->game) {
    free(stats->game);
  }
  free(stats);
}

int read_scores(FILE *f, Score *score) {
  return read_csv(f, "tsiii", &score->timestamp, &score->game, &score->victory, &score->score, &score->duration);
}
