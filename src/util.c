/* csol
 * Copyright (c) 2017 Niels Sonnich Poulsen (http://nielssp.dk)
 * Licensed under the MIT license.
 * See the LICENSE file or http://opensource.org/licenses/MIT for more information.
 */

#define _XOPEN_SOURCE 500

#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <libgen.h>
#include <errno.h>
#if defined(MSDOS) || defined(USE_DIRECT)
#include <direct.h>
#elif defined(_WIN32)
#include <io.h>
#endif

int file_exists(const char *path) {
  FILE *f = fopen(path, "r");
  if (f) {
    fclose(f);
    return 1;
  }
  return 0;
}

char *combine_paths(const char *path1, const char *path2) {
  size_t length1 = strlen(path1);
  size_t length2 = strlen(path2);
  size_t combined_length = length1 + length2 + 2;
  char *combined_path = malloc(combined_length);
  memcpy(combined_path, path1, length1);
  if (path1[length1 - 1] != PATH_SEP) {
    combined_path[length1++] = PATH_SEP;
  }
  memcpy(combined_path + length1, path2, length2);
  combined_path[length1 + length2] = '\0';
  return combined_path;
}

char *find_data_file(const char *name, const char *arg0) {
  char *path = NULL;
#ifdef USE_XDG_PATHS
    char *data_dir = getenv("XDG_DATA_HOME");
    if (data_dir) {
      char *combined_data_dir = combine_paths(data_dir, "csol");
      if (mkdir_rec(combined_data_dir)) {
        path = combine_paths(combined_data_dir, name);
      }
      free(combined_data_dir);
    } else {
      data_dir = getenv("HOME");
      if (data_dir) {
        char *combined_data_dir = combine_paths(data_dir, ".local/share/csol");
        if (mkdir_rec(combined_data_dir)) {
          path = combine_paths(combined_data_dir, name);
        }
        free(combined_data_dir);
      }
    }
#else
  char *copy = strdup(arg0);
  path = combine_paths(dirname(copy), name);
  free(copy);
#endif
  return path;
}

char *find_system_config_file(const char *name) {
#ifdef USE_XDG_PATHS
  FILE *f;
  char *config_file;
  char *config_dir = getenv("XDG_CONFIG_DIRS");
  char *relative_path = combine_paths("csol", name);
  if (config_dir) {
    int i = 0;
    while (1) {
      if (!config_dir[i] || config_dir[i] == ':') {
        char *dir = malloc(i + 1);
        strncpy(dir, config_dir, i);
        dir[i] = '\0';
        config_file = combine_paths(dir, relative_path);
        f = fopen(config_file, "r");
        free(dir);
        if (f) {
          fclose(f);
          free(relative_path);
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
    free(relative_path);
  } else {
    config_file = combine_paths("/etc/xdg", relative_path);
    free(relative_path);
    f = fopen(config_file, "r");
    if (f) {
      fclose(f);
      return config_file;
    }
  }
#endif
  return NULL;
}

static int check_dir(const char *path) {
  struct stat stat_buffer;
  if (stat(path, &stat_buffer) == 0 && S_ISDIR(stat_buffer.st_mode)) {
    return 1;
  }
#if defined(MSDOS) || defined(USE_DIRECT) || defined(_WIN32)
  if (_mkdir(path) == 0) {
    return 1;
  }
#else
  if (mkdir(path, S_IRWXU) == 0) {
    return 1;
  }
#endif
  printf("%s: Creating directory failed: %s\n", path, strerror(errno));
  return 0;
}

int mkdir_rec(const char *path) {
  size_t length = strlen(path);
  char *buffer = malloc(length + 1);
  char *p;
  memcpy(buffer, path, length + 1);
  if (buffer[length - 1] == PATH_SEP) {
    buffer[length - 1] = 0;
  }
  for (p = buffer + 1; *p; p++) {
    if (*p == PATH_SEP) {
      *p = 0;
      if (!check_dir(buffer)) {
        free(buffer);
        return 0;
      }
      *p = PATH_SEP;
    }
  }
  if (!check_dir(buffer)) {
    free(buffer);
    return 0;
  }
  free(buffer);
  return 1;
}
