/* csol
 * Copyright (c) 2020 Niels Sonnich Poulsen (http://nielssp.dk)
 * Licensed under the MIT license.
 * See the LICENSE file or http://opensource.org/licenses/MIT for more information.
 */

#ifndef SCORES_H
#define SCORES_H

#include <time.h>
#include <stdio.h>
#include <inttypes.h>

typedef struct stats Stats;
typedef struct score Score;

struct stats {
  Stats *next;
  char *game;
  int32_t times_played;
  int32_t times_won;
  int32_t total_time_played;
  int32_t best_time;
  int32_t best_score;
  time_t first_played;
  time_t last_played;
};

struct score {
  time_t timestamp;
  char *game;
  int32_t victory;
  int32_t score;
  int32_t duration;
};

extern int scores_enabled;
extern char *scores_file_path;

extern int stats_enabled;
extern char *stats_file_path;

extern int show_score;

void register_scores_file(const char *cwd, const char *file_name);
int touch_scores_file(const char *arg0);

void register_stats(const char *cwd, const char *file_name);
int touch_stats_file(const char *arg0);

int append_score(const char *game_name, int victory, int32_t score, int32_t duration, Stats *stats);

Stats *get_stats();
void put_stats(Stats *stats);
void delete_stats(Stats *stats);

int read_scores(FILE *f, Score *score);

#endif
