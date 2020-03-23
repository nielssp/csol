/* csol
 * Copyright (c) 2020 Niels Sonnich Poulsen (http://nielssp.dk)
 * Licensed under the MIT license.
 * See the LICENSE file or http://opensource.org/licenses/MIT for more information.
 */

#ifndef SCORES_H
#define SCORES_H

#include <time.h>

typedef struct stats Stats;

struct stats {
  Stats *next;
  char *game;
  int times_played;
  int times_won;
  int total_time_played;
  int best_time;
  int best_score;
  time_t first_played;
  time_t last_played;
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

int append_score(const char *game_name, int victory, int score, int duration, Stats *stats);

Stats *get_stats();
void delete_stats(Stats *stats);

#endif
