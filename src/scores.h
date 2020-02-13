/* csol
 * Copyright (c) 2017 Niels Sonnich Poulsen (http://nielssp.dk)
 * Licensed under the MIT license.
 * See the LICENSE file or http://opensource.org/licenses/MIT for more information.
 */

#ifndef SCORES_H
#define SCORES_H

#include <time.h>

extern int scores_enabled;
extern char *scores_file_path;

extern int stats_enabled;
extern char *stats_file_path;

void register_scores_file(const char *cwd, const char *file_name);
int touch_scores_file(const char *arg0);

void register_stats(const char *cwd, const char *file_name);
int touch_stats_file(const char *arg0);

int append_score(const char *game_name, int victory, int score, time_t start_time);

#endif
