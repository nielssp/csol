#include <stdlib.h>
#include <string.h>

#include "theme.h"

ThemeList *first_theme = NULL;
ThemeList *last_theme = NULL;

Theme *new_theme() {
  Theme *theme = malloc(sizeof(Theme));
  // TODO: defaults
  return theme;
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

ThemeList *list_themes() {
  return first_theme;
}

Theme *get_theme(const char *name) {
  for (ThemeList *themes = list_themes(); themes; themes = themes->next) {
    if (strcmp(themes->theme->name, name) == 0) {
      return themes->theme;
    }
  }
  return NULL;
}
