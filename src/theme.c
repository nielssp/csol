/* csol
 * Copyright (c) 2017 Niels Sonnich Poulsen (http://nielssp.dk)
 * Licensed under the MIT license.
 * See the LICENSE file or http://opensource.org/licenses/MIT for more information.
 */

#define _XOPEN_SOURCE 500

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

static void init_default_ranks(char **ranks) {
  ranks[0] = strdup("A");
  ranks[1] = strdup("2");
  ranks[2] = strdup("3");
  ranks[3] = strdup("4");
  ranks[4] = strdup("5");
  ranks[5] = strdup("6");
  ranks[6] = strdup("7");
  ranks[7] = strdup("8");
  ranks[8] = strdup("9");
  ranks[9] = strdup("10");
  ranks[10] = strdup("J");
  ranks[11] = strdup("Q");
  ranks[12] = strdup("K");
}

Theme *new_theme() {
  Theme *theme = malloc(sizeof(Theme));
  theme->name = NULL;
  theme->title = NULL;
  theme->heart = NULL;
  theme->spade = NULL;
  theme->diamond = NULL;
  theme->club = NULL;
  theme->ranks = malloc(13 * sizeof(char *));
  init_default_ranks(theme->ranks);
  theme->width = 6;
  theme->height = 4;
  theme->x_spacing = 2;
  theme->y_spacing = 1;
  theme->x_margin = 2;
  theme->y_margin = 1;
  theme->utf8 = 1;
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

Text init_text() {
  Text t;
  t.next = NULL;
  t.format = TEXT_NONE;
  t.x = 0;
  t.y = 0;
  t.align_right = 0;
  return t;
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
      first_theme = NULL;
      last_theme = NULL;
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

#ifdef USE_PDCURSES

static unsigned char convert_code_point(int code_point) {
  switch (code_point) {
    case 0x263A: return 1;
    case 0x263B: return 2;
    case 0x2665: return 3;
    case 0x2666: return 4;
    case 0x2663: return 5;
    case 0x2660: return 6;
    case 0x2022: return 7;
    case 0x25D8: return 8;
    case 0x25CB: return 9;
    case 0x25D9: return 10;
    case 0x2642: return 11;
    case 0x2640: return 12;
    case 0x266A: return 13;
    case 0x266B: return 14;
    case 0x263C: return 15;
    case 0x25BA: return 16;
    case 0x25C4: return 17;
    case 0x2195: return 18;
    case 0x203C: return 19;
    case 0x00B6: return 20;
    case 0x00A7: return 21;
    case 0x25AC: return 22;
    case 0x21A8: return 23;
    case 0x2191: return 24;
    case 0x2193: return 25;
    case 0x2192: return 26;
    case 0x2190: return 27;
    case 0x221F: return 28;
    case 0x2194: return 29;
    case 0x25B2: return 30;
    case 0x25BC: return 31;
    case 0x2302: return 127;
    case 0x2591: return 176;
    case 0x2592: return 177;
    case 0x2593: return 178;
    case 0x2502: return 179;
    case 0x2524: return 180;
    case 0x2561: return 181;
    case 0x2562: return 182;
    case 0x2556: return 183;
    case 0x2555: return 184;
    case 0x2563: return 185;
    case 0x2551: return 186;
    case 0x2557: return 187;
    case 0x255D: return 188;
    case 0x255C: return 189;
    case 0x255B: return 190;
    case 0x2510: return 191;
    case 0x2514: return 192;
    case 0x2534: return 193;
    case 0x252C: return 194;
    case 0x251C: return 195;
    case 0x2500: return 196;
    case 0x253C: return 197;
    case 0x255E: return 198;
    case 0x255F: return 199;
    case 0x255A: return 200;
    case 0x2554: return 201;
    case 0x2569: return 202;
    case 0x2566: return 203;
    case 0x2560: return 204;
    case 0x2550: return 205;
    case 0x256C: return 206;
    case 0x2567: return 207;
    case 0x2568: return 208;
    case 0x2564: return 209;
    case 0x2565: return 210;
    case 0x2559: return 211;
    case 0x2558: return 212;
    case 0x2552: return 213;
    case 0x2553: return 214;
    case 0x256B: return 215;
    case 0x256A: return 216;
    case 0x2518: return 217;
    case 0x250C: return 218;
    case 0x2588: return 219;
    case 0x2584: return 220;
    case 0x258C: return 221;
    case 0x2590: return 222;
    case 0x2580: return 223;
    default:
      printf("Unknown code point: %04x\n", code_point);
      return '?';
  }
}

static void convert_string(char *str) {
  char *s;
  int next = 0;
  int code_point = 0;
  if (!str) {
    return;
  }
  for (s = str ; *s; s++) {
    if (*s & 0x80) {
      if (*s & 0x40) {
        if (code_point) {
          str[next++] = convert_code_point(code_point);
          code_point = 0;
        }
        if ((*s & 0xE0) == 0xC0) {
          code_point = *s & 0x1F;
        } else if ((*s & 0xF0) == 0xE0) {
          code_point = *s & 0x0F;
        } else if ((*s & 0xF8) == 0xF0) {
          code_point = *s & 0x07;
        }
      } else {
        code_point <<= 6;
        code_point |= *s & 0x3F;
      }
    } else {
      if (code_point) {
        str[next++] = convert_code_point(code_point);
        code_point = 0;
      }
      str[next++] = *s;
    }
  }
  if (code_point) {
    str[next++] = convert_code_point(code_point);
    code_point = 0;
  }
  str[next] = 0;
}

static void convert_layout(Layout *layout) {
  convert_string(layout->top);
  convert_string(layout->middle);
  convert_string(layout->bottom);
}

#endif

void convert_theme(Theme *theme) {
#ifdef USE_PDCURSES
  if (!theme->utf8) {
    return;
  }
  theme->utf8 = 0;
  convert_string(theme->heart);
  convert_string(theme->spade);
  convert_string(theme->diamond);
  convert_string(theme->club);
  convert_layout(&theme->empty_layout);
  convert_layout(&theme->back_layout);
  convert_layout(&theme->red_layout);
  convert_layout(&theme->black_layout);
#endif
}
