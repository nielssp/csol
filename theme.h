#ifndef THEME_H
#define THEME_H

typedef struct color Color;
typedef struct color_pair ColorPair;
typedef struct text Text;
typedef struct layout Layout;
typedef struct theme Theme;
typedef struct theme_list ThemeList;

struct color {
  short color;
  short r;
  short g;
  short b;
};

struct color_pair {
  short fg;
  short bg;
};

enum text_format {
  RANK,
  SUIT,
  RANK_SUIT,
  SUIT_RANK
};

struct text {
  enum text_format format;
  short x;
  short y;
  short align_right;
};

struct layout {
  ColorPair color;
  char *top;
  char *middle;
  char *bottom;
  Text *text_fields;
};

struct theme {
  char *name;
  char *title;
  char *heart;
  char *spade;
  char *diamond;
  char *club;
  int width;
  int height;
  int padding;
  Color *colors;
  Layout empty_layout;
  Layout back_layout;
  Layout red_layout;
  Layout black_layout;
};

struct theme_list {
  Theme *theme;
  ThemeList *next;
};

Theme *new_theme();

void register_theme(Theme *theme);

ThemeList *list_themes();

Theme *get_theme(const char *name);

#endif
