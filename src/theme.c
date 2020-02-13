/* csol
 * Copyright (c) 2017 Niels Sonnich Poulsen (http://nielssp.dk)
 * Licensed under the MIT license.
 * See the LICENSE file or http://opensource.org/licenses/MIT for more information.
 */

#include "theme.h"

#include "util.h"
#include "rc.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct dir_list {
  char *dir;
  struct dir_list *next;
};

ThemeList *first_theme = NULL;
ThemeList *last_theme = NULL;

struct dir_list *theme_dirs = NULL;

Theme *new_theme() {
  Theme *theme = malloc(sizeof(Theme));
  theme->name = NULL;
  theme->title = NULL;
  theme->heart = NULL;
  theme->spade = NULL;
  theme->diamond = NULL;
  theme->club = NULL;
  theme->width = 6;
  theme->height = 4;
  theme->x_spacing = 2;
  theme->y_spacing = 1;
  theme->x_margin = 2;
  theme->y_margin = 1;
  theme->colors = NULL;
  theme->background.fg = 7;
  theme->background.fg_name = NULL;
  theme->background.bg = 0;
  theme->background.bg_name = NULL;
  theme->empty_layout = init_layout();
  theme->back_layout = init_layout();
  theme->red_layout = init_layout();
  theme->black_layout = init_layout();
  return theme;
}

Layout init_layout() {
  Layout l;
  l.color.fg = 7;
  l.color.fg_name = NULL;
  l.color.bg = 0;
  l.color.bg_name = NULL;
  l.top = NULL;
  l.middle = NULL;
  l.bottom = NULL;
  l.left_padding = 1;
  l.right_padding = 1;
  l.text_fields = NULL;
  return l;
}

void define_color(Theme *theme, char *name, short index, short red, short green, short blue) {
  Color *color = malloc(sizeof(Color));
  color->next = theme->colors;
  color->name = name;
  color->index = index;
  color->red = red;
  color->green = green;
  color->blue = blue;
  theme->colors = color;
}

void register_theme(Theme *theme) {
  if (theme->name) {
    ThemeList *next = malloc(sizeof(struct theme_list));
    next->theme = theme;
    next->next = NULL;
    if (last_theme) {
      last_theme->next = next;
      last_theme = next;
    } else {
      first_theme = last_theme = next;
    }
  }
}

void register_theme_dir(const char *cwd, const char *dir) {
  struct dir_list *theme_dir = malloc(sizeof(struct dir_list));
  theme_dir->dir = combine_paths(cwd, dir);
  theme_dir->next = theme_dirs;
  theme_dirs = theme_dir;
}

void load_theme_dirs() {
  struct dir_list *current = theme_dirs;
  while (current) {
    struct dir_list *next;
    execute_dir(current->dir);
    next = current->next;
    free(current->dir);
    free(current);
    current = next;
  }
  theme_dirs = NULL;
}

ThemeList *list_themes() {
  return first_theme;
}

Theme *get_theme_in_list(const char *name) {
  ThemeList *themes;
  for (themes = list_themes(); themes; themes = themes->next) {
    if (strcmp(themes->theme->name, name) == 0) {
      return themes->theme;
    }
  }
  return NULL;
}

Theme *get_theme(const char *name) {
  struct dir_list *theme_dir;
  Theme *theme = get_theme_in_list(name);
  if (theme) {
    return theme;
  }
  for (theme_dir = theme_dirs; theme_dir; theme_dir = theme_dir->next) {
    char *theme_path = combine_paths(theme_dir->dir, name);
    if (file_exists(theme_path)) {
      execute_file(theme_path);
      theme = get_theme_in_list(name);
      if (theme) {
        free(theme_path);
        return theme;
      }
    }
    free(theme_path);
  }
  printf("Warning: file \"%s\" not found, searching all theme files\n", name);
  load_theme_dirs();
  return get_theme_in_list(name);
}

char *card_suit(Card *card, Theme *theme) {
  switch (card->suit) {
    case HEART:
      return theme->heart;
    case SPADE:
      return theme->spade;
    case DIAMOND:
      return theme->diamond;
    case CLUB:
      return theme->club;
  }
  return "";
}
