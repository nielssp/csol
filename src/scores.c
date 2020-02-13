/* csol
 * Copyright (c) 2020 Niels Sonnich Poulsen (http://nielssp.dk)
 * Licensed under the MIT license.
 * See the LICENSE file or http://opensource.org/licenses/MIT for more information.
 */

#include "scores.h"

#include "util.h"

#include <stdlib.h>
#include <stdio.h>
#define _XOPEN_SOURCE 500
#include <string.h>
#include <libgen.h>

int scores_enabled = 0;
char *scores_file_path = NULL;

int touch_scores_file(const char *arg0) {
  FILE *f;
  if (!scores_enabled) {
    return 1;
  }
  if (!scores_file_path) {
#ifdef USE_XDG_PATHS
    char *data_dir = getenv("XDG_DATA_HOME");
    if (data_dir) {
      char *combined_data_dir = combine_paths(data_dir, "csol");
      if (mkdir_rec(combined_data_dir)) {
        scores_file_path = combine_paths(combined_data_dir, "scores.csv");
      }
      free(combined_data_dir);
    } else {
      data_dir = getenv("HOME");
      if (data_dir) {
        char *combined_data_dir = combine_paths(data_dir, ".local/share/csol");
        if (mkdir_rec(combined_data_dir)) {
          scores_file_path = combine_paths(combined_data_dir, "scores.csv");
        }
        free(combined_data_dir);
      }
    }
#else
    char *copy = strdup(arg0);
    scores_file_path = combine_paths(dirname(copy), "scores.csv");
    free(copy);
#endif
  }
  if (!scores_file_path) {
    return 0;
  }
  f = fopen(scores_file_path, "r+");
  if (!f) {
    f = fopen(scores_file_path, "w");
    if (!f) {
      return 0;
    }
  }
  fclose(f);
  return 1;
}

int append_score(const char *game_name, int victory, int score, time_t start_time) {
  FILE *f;
  struct tm *utc;
  time_t now;
  char date[26];
  if (!scores_file_path) {
    return 1;
  }
  f = fopen(scores_file_path, "a");
  if (!f) {
    return 0;
  }
  now = time(NULL);
  utc = gmtime(&now);
  if (utc) {
    if (strftime(date, sizeof(date), "%Y-%m-%dT%H:%M:%SZ", utc)) {
      fprintf(f, "%s,%s,%d,%d,%d\n", date, game_name, victory, score, (int)(now - start_time));
    }
  } else {
      printf("localtime fail\n");
  }
  fclose(f);
  return 1;
}
