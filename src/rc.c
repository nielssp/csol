/* csol
 * Copyright (c) 2017 Niels Sonnich Poulsen (http://nielssp.dk)
 * Licensed under the MIT license.
 * See the LICENSE file or http://opensource.org/licenses/MIT for more information.
 */

#define _XOPEN_SOURCE 500

#include "rc.h"

#include "theme.h"
#include "game.h"
#include "util.h"
#include "scores.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <errno.h>
#include <libgen.h>
#if defined(MSDOS) || defined(USE_DIRECT)
#include <direct.h>
#else
#include <dirent.h>
#endif

#define BUFFER_INC 32

typedef enum {
  K_END_OF_BLOCK,
  K_UNDEFINED,
  K_THEME,
  K_GAME,
  K_NAME,
  K_TITLE,
  K_HEART,
  K_DIAMOND,
  K_SPADE,
  K_CLUB,
  K_WIDTH,
  K_HEIGHT,
  K_TOP,
  K_MIDDLE,
  K_BOTTOM,
  K_EMPTY,
  K_BACK,
  K_RED,
  K_BLACK,
  K_COLOR,
  K_FG,
  K_BG,
  K_REPEAT,
  K_FOUNDATION,
  K_TABLEAU,
  K_STOCK,
  K_WASTE,
  K_CELL,
  K_X,
  K_Y,
  K_DEAL,
  K_HIDE,
  K_FIRST_RANK,
  K_FIRST_SUIT,
  K_NEXT_RANK,
  K_NEXT_SUIT,
  K_MOVE_GROUP,
  K_FROM,
  K_WIN_RANK,
  K_REDEAL,
  K_X_SPACING,
  K_Y_SPACING,
  K_X_MARGIN,
  K_Y_MARGIN,
  K_LEFT_PADDING,
  K_RIGHT_PADDING,
  K_INCLUDE,
  K_GAME_DIR,
  K_THEME_DIR,
  K_DEFAULT_GAME,
  K_DEFAULT_THEME,
  K_CLASS,
  K_SAME_CLASS,
  K_SCORES,
  K_SCORES_FILE,
  K_STATS,
  K_STATS_FILE,
  K_SHOW_SCORE,
  K_RANK,
  K_TEXT,
  K_ALIGN,
  K_FORMAT,
  K_UTF8
} Keyword;

struct symbol {
  char *symbol;
  Keyword keyword;
};

struct symbol root_commands[] = {
  {"theme", K_THEME},
  {"game", K_GAME},
  {"include", K_INCLUDE},
  {"theme_dir", K_THEME_DIR},
  {"game_dir", K_GAME_DIR},
  {"default_game", K_DEFAULT_GAME},
  {"default_theme", K_DEFAULT_THEME},
  {"scores", K_SCORES},
  {"scores_file", K_SCORES_FILE},
  {"stats", K_STATS},
  {"stats_file", K_STATS_FILE},
  {"show_score", K_SHOW_SCORE},
  {NULL, K_UNDEFINED}
};

struct symbol theme_commands[] ={
  {"name", K_NAME},
  {"title", K_TITLE},
  {"heart", K_HEART},
  {"diamond", K_DIAMOND},
  {"spade", K_SPADE},
  {"club", K_CLUB},
  {"width", K_WIDTH},
  {"height", K_HEIGHT},
  {"empty", K_EMPTY},
  {"back", K_BACK},
  {"red", K_RED},
  {"black", K_BLACK},
  {"color", K_COLOR},
  {"x_spacing", K_X_SPACING},
  {"y_spacing", K_Y_SPACING},
  {"x_margin", K_X_MARGIN},
  {"y_margin", K_Y_MARGIN},
  {"fg", K_FG},
  {"bg", K_BG},
  {"rank", K_RANK},
  {"utf8", K_UTF8},
  {NULL, K_UNDEFINED}
};

struct symbol layout_commands[] = {
  {"top", K_TOP},
  {"middle", K_MIDDLE},
  {"bottom", K_BOTTOM},
  {"fg", K_FG},
  {"bg", K_BG},
  {"left_padding", K_LEFT_PADDING},
  {"right_padding", K_RIGHT_PADDING},
  {"text", K_TEXT},
  {NULL, K_UNDEFINED}
};

struct symbol text_commands[] = {
  {"x", K_X},
  {"y", K_Y},
  {"align", K_ALIGN},
  {"format", K_FORMAT},
  {NULL, K_UNDEFINED}
};

struct symbol game_commands[] ={
  {"name", K_NAME},
  {"title", K_TITLE},
  {"repeat", K_REPEAT},
  {"tableau", K_TABLEAU},
  {"foundation", K_FOUNDATION},
  {"cell", K_CELL},
  {"stock", K_STOCK},
  {"waste", K_WASTE},
  {NULL, K_UNDEFINED}
};

struct symbol game_rule_commands[] ={
  {"x", K_X},
  {"y", K_Y},
  {"deal", K_DEAL},
  {"redeal", K_REDEAL},
  {"hide", K_HIDE},
  {"first_rank", K_FIRST_RANK},
  {"first_suit", K_FIRST_SUIT},
  {"next_rank", K_NEXT_RANK},
  {"next_suit", K_NEXT_SUIT},
  {"move_group", K_MOVE_GROUP},
  {"from", K_FROM},
  {"win_rank", K_WIN_RANK},
  {"class", K_CLASS},
  {"same_class", K_SAME_CLASS},
  {NULL, K_UNDEFINED}
};

struct property {
  char *name;
  char *value;
  struct property *next;
};

struct position {
  long offset;
  int line;
  int column;
};

const char *current_file = NULL;
int line = 0;
int column = 0;

struct property *properties = NULL;

int previous_column = 0;

int has_error = 0;

int read_char(FILE *file) {
  int c = fgetc(file);
  if (c == '\n') {
    line++;
    previous_column = column;
    column = 1;
  } else {
    column++;
  }
  return c;
}

void unread_char(int c, FILE *file) {
  ungetc(c, file);
  column--;
  if (column <= 0) {
    column = previous_column;
    line--;
  }
}

struct position get_position(FILE *file) {
  struct position p;
  p.offset = ftell(file);
  p.line = line;
  p.column = column;
  return p;
}

void set_position(struct position pos, FILE *file) {
  fseek(file, pos.offset, SEEK_SET);
  line = pos.line;
  column = pos.column;
}

void rc_error(const char *format, ...) {
  va_list va;
  has_error = 1;
  if (current_file) {
    printf("%s:%d:%d: error: ", current_file, line, column);
  }
  va_start(va, format);
  vprintf(format, va);
  va_end(va);
  printf("\n");
}

void skip_whitespace(FILE *file) {
  int c;
  while (isspace(c = read_char(file)) || c == '#') {
    if (c == '#') {
      while ((c = read_char(file)) != '\n' && c != EOF) { }
    }
  }
  unread_char(c, file);
}

char *resize_buffer(char *buffer, size_t oldsize, size_t newsize) {
  char *new = NULL;
  if (newsize < oldsize) {
    return NULL;
  }
  new = (char *)malloc(newsize);
  if (!new) {
    return NULL;
  }
  memcpy(new, buffer, oldsize);
  free(buffer);
  return new;
}

char *read_symbol(FILE *file) {
  size_t size = BUFFER_INC, i = 0;
  char *buffer;
  int c = read_char(file);
  if (!isalpha(c)) {
    unread_char(c, file);
    return NULL;
  }
  buffer = malloc(size);
  while (isalnum(c) || c == '_') {
    buffer[i++] = c;
    if (i >= size) {
      buffer = resize_buffer(buffer, size, size + BUFFER_INC);
      size = size + BUFFER_INC;
    }
    c = read_char(file);
  }
  unread_char(c, file);
  buffer[i] = '\0';
  return buffer;
}

char *read_quoted(FILE *file) {
  size_t size = BUFFER_INC, i = 0;
  int c = read_char(file);
  char *buffer = malloc(size);
  while (c != EOF && c != '"') {
    buffer[i++] = c;
    if (i >= size) {
      buffer = resize_buffer(buffer, size, size + BUFFER_INC);
      size = size + BUFFER_INC;
    }
    c = read_char(file);
  }
  buffer[i] = '\0';
  return buffer;
}

char *read_line(FILE *file) {
  size_t size = BUFFER_INC, i = 0;
  int c = read_char(file);
  char *buffer = malloc(size);
  while (c != EOF && c != '\r' && c != '\n' && c != '#') {
    buffer[i++] = c;
    if (i >= size) {
      buffer = resize_buffer(buffer, size, size + BUFFER_INC);
      size = size + BUFFER_INC;
    }
    c = read_char(file);
  }
  if (c == '#') {
    unread_char(c, file);
  } else if (c == '\r') {
    c = read_char(file);
    if (c != '\n') {
      unread_char(c, file);
    }
  }
  buffer[i] = '\0';
  return buffer;
}

char *read_value(FILE *file) {
  int c;
  skip_whitespace(file);
  c = read_char(file);
  if (c == '"') {
    return read_quoted(file);
  }
  unread_char(c, file);
  return read_line(file);
}

void redefine_property(char **property, FILE *file) {
  char *value = read_value(file);
  if (*property) {
    free(*property);
  }
  *property = value;
}

int read_int(FILE *file) {
  int c;
  int sign, value;
  skip_whitespace(file);
  c = read_char(file);
  sign = 1;
  value = 0;
  if (c == '-') {
    sign = -1;
    c = read_char(file);
  }
  if (!isdigit(c)) {
    unread_char(c, file);
    return 0;
  }
  while (isdigit(c)) {
    value = value * 10 + c - '0';
    c = read_char(file);
  }
  unread_char(c, file);
  return value * sign;
}

int read_expr(FILE *file, int index) {
  int increment;
  int value = read_int(file);
  int c = read_char(file);
  if (c != '+') {
    unread_char(c, file);
    return value;
  }
  increment = read_int(file);
  if (increment == 0) {
    increment = 1;
  }
  return value + increment * index;
}

int read_color(FILE *file, char **name) {
  int c;
  skip_whitespace(file);
  c = read_char(file);
  unread_char(c, file);
  if (isdigit(c) || c == '-') {
    return read_int(file);
  }
  *name = read_symbol(file);
  return -1;
}

Keyword read_command(FILE *file, struct symbol *commands) {
  char *keyword;
  skip_whitespace(file);
  keyword = read_symbol(file);
  if (!keyword) {
    return K_END_OF_BLOCK;
  }
  while (commands->symbol) {
    if (strcmp(commands->symbol, keyword) == 0) {
      free(keyword);
      return commands->keyword;
    }
    commands++;
  }
  rc_error("undefined keyword: %s", keyword);
  free(keyword);
  return K_UNDEFINED;
}

int expect(int expected, FILE *file) {
  int actual = read_char(file);
  if (actual != expected) {
    rc_error("unexpected '%c', expected '%c'", actual, expected);
    return 0;
  }
  return 1;
}

int begin_block(FILE *file) {
  skip_whitespace(file);
  return expect('{', file);
}

int end_block(FILE *file) {
  skip_whitespace(file);
  return expect('}', file);
}

TextFormat read_text_format(FILE *file) {
  char *symbol = read_value(file);
  TextFormat format = TEXT_NONE;
  if (strcmp(symbol, "rank_suit") == 0) {
    format = TEXT_RANK_SUIT;
  } else if (strcmp(symbol, "suit_rank") == 0) {
    format = TEXT_SUIT_RANK;
  } else if (strcmp(symbol, "suit") == 0) {
    format = TEXT_SUIT;
  } else if (strcmp(symbol, "rank") == 0) {
    format = TEXT_RANK;
  }
  free(symbol);
  return format;
}

int read_text_align(FILE *file) {
  char *symbol = read_value(file);
  int align = 0;
  if (strcmp(symbol, "right") == 0) {
    align = 1;
  }
  free(symbol);
  return align;
}

Text define_text(FILE *file) {
  Keyword command;
  Text text = init_text();
  begin_block(file);
  while ((command = read_command(file, text_commands))) {
    switch (command) {
      case K_FORMAT:
        text.format = read_text_format(file);
        break;
      case K_X:
        text.x = read_int(file);
        break;
      case K_Y:
        text.y = read_int(file);
        break;
      case K_ALIGN:
        text.align_right = read_text_align(file);
        break;
      default:
        break;
    }
  }
  end_block(file);
  return text;
}

Layout define_layout(FILE *file) {
  Keyword command;
  Layout layout = init_layout();
  begin_block(file);
  while ((command = read_command(file, layout_commands))) {
    switch (command) {
      case K_TOP:
        redefine_property(&layout.top, file);
        break;
      case K_MIDDLE:
        redefine_property(&layout.middle, file);
        break;
      case K_BOTTOM:
        redefine_property(&layout.bottom, file);
        break;
      case K_FG:
        layout.color.fg = read_color(file, &layout.color.fg_name);
        break;
      case K_BG:
        layout.color.bg = read_color(file, &layout.color.bg_name);
        break;
      case K_LEFT_PADDING:
        layout.left_padding = read_int(file);
        break;
      case K_RIGHT_PADDING:
        layout.right_padding = read_int(file);
        break;
      case K_TEXT: {
        Text text = define_text(file);
        text.next = layout.text_fields;
        layout.text_fields = malloc(sizeof(Text));
        *layout.text_fields = text;
        break;
      }
      default:
        break;
    }
  }
  end_block(file);
  return layout;
}

void define_theme(FILE *file) {
  Keyword command;
  Theme *theme = new_theme();
  begin_block(file);
  while ((command = read_command(file, theme_commands))) {
    switch (command) {
      case K_NAME:
        redefine_property(&theme->name, file);
        break;
      case K_TITLE:
        redefine_property(&theme->title, file);
        break;
      case K_HEART:
        redefine_property(&theme->heart, file);
        break;
      case K_DIAMOND:
        redefine_property(&theme->diamond, file);
        break;
      case K_SPADE:
        redefine_property(&theme->spade, file);
        break;
      case K_CLUB:
        redefine_property(&theme->club, file);
        break;
      case K_WIDTH:
        theme->width = read_int(file);
        break;
      case K_HEIGHT:
        theme->height = read_int(file);
        break;
      case K_EMPTY:
        theme->empty_layout = define_layout(file);
        break;
      case K_BACK:
        theme->back_layout = define_layout(file);
        break;
      case K_RED:
        theme->red_layout = define_layout(file);
        break;
      case K_BLACK:
        theme->black_layout = define_layout(file);
        break;
      case K_X_SPACING:
        theme->x_spacing = read_int(file);
        break;
      case K_Y_SPACING:
        theme->y_spacing = read_int(file);
        break;
      case K_X_MARGIN:
        theme->x_margin = read_int(file);
        break;
      case K_Y_MARGIN:
        theme->y_margin = read_int(file);
        break;
      case K_COLOR: {
        char *name = NULL;
        short index = 0;
        short red, green, blue;
        int c;
        skip_whitespace(file);
        c = read_char(file);
        unread_char(c, file);
        if (isdigit(c)) {
          index = read_int(file);
        } else {
          name = read_symbol(file);
          if (!name) {
            break;
          }
        }
        red = read_int(file);
        green = read_int(file);
        blue = read_int(file);
        define_color(theme, name, index, red, green, blue);
        break;
      }
      case K_FG:
        theme->background.fg = read_color(file, &theme->background.fg_name);
        break;
      case K_BG:
        theme->background.bg = read_color(file, &theme->background.bg_name);
        break;
      case K_RANK: {
        int rank = read_int(file);
        char *symbol = read_value(file);
        if (rank >= 1 && rank <= 13) {
          free(theme->ranks[rank - 1]);
          theme->ranks[rank - 1] = symbol;
        } else {
          free(symbol);
        }
        break;
      }
      case K_UTF8:
        theme->utf8 = read_int(file);
        break;
      default:
        break;
    }
  }
  end_block(file);
  register_theme(theme);
}

GameRuleSuit read_suit(FILE *file) {
  char *symbol = read_value(file);
  GameRuleSuit suit = SUIT_NONE;
  if (strcmp(symbol, "any") == 0) {
    suit = SUIT_ANY;
  } else if (strcmp(symbol, "heart") == 0) {
    suit = SUIT_HEART;
  } else if (strcmp(symbol, "diamond") == 0) {
    suit = SUIT_DIAMOND;
  } else if (strcmp(symbol, "spade") == 0) {
    suit = SUIT_SPADE;
  } else if (strcmp(symbol, "club") == 0) {
    suit = SUIT_CLUB;
  } else if (strcmp(symbol, "red") == 0) {
    suit = SUIT_RED;
  } else if (strcmp(symbol, "black") == 0) {
    suit = SUIT_BLACK;
  } else if (strcmp(symbol, "same") == 0) {
    suit = SUIT_SAME;
  } else if (strcmp(symbol, "same_color") == 0) {
    suit = SUIT_SAME_COLOR;
  } else if (strcmp(symbol, "diff") == 0) {
    suit = SUIT_DIFF;
  } else if (strcmp(symbol, "diff_color") == 0) {
    suit = SUIT_DIFF_COLOR;
  }
  free(symbol);
  return suit;
}

GameRuleRank read_rank(FILE *file) {
  char *symbol = read_value(file);
  GameRuleRank rank = RANK_NONE;
  if (strcmp(symbol, "any") == 0) {
    rank = RANK_ANY;
  } else if (strcmp(symbol, "a") == 0) {
    rank = RANK_ACE;
  } else if (strcmp(symbol, "2") == 0) {
    rank = RANK_2;
  } else if (strcmp(symbol, "3") == 0) {
    rank = RANK_3;
  } else if (strcmp(symbol, "4") == 0) {
    rank = RANK_4;
  } else if (strcmp(symbol, "5") == 0) {
    rank = RANK_5;
  } else if (strcmp(symbol, "6") == 0) {
    rank = RANK_6;
  } else if (strcmp(symbol, "7") == 0) {
    rank = RANK_7;
  } else if (strcmp(symbol, "8") == 0) {
    rank = RANK_8;
  } else if (strcmp(symbol, "9") == 0) {
    rank = RANK_9;
  } else if (strcmp(symbol, "10") == 0) {
    rank = RANK_10;
  } else if (strcmp(symbol, "j") == 0) {
    rank = RANK_JACK;
  } else if (strcmp(symbol, "q") == 0) {
    rank = RANK_QUEEN;
  } else if (strcmp(symbol, "k") == 0) {
    rank = RANK_KING;
  } else if (strcmp(symbol, "same") == 0) {
    rank = RANK_SAME;
  } else if (strcmp(symbol, "down") == 0) {
    rank = RANK_DOWN;
  } else if (strcmp(symbol, "up") == 0) {
    rank = RANK_UP;
  } else if (strcmp(symbol, "up_down") == 0) {
    rank = RANK_UP_DOWN;
  } else if (strcmp(symbol, "lower") == 0) {
    rank = RANK_LOWER;
  } else if (strcmp(symbol, "higher") == 0) {
    rank = RANK_HIGHER;
  }
  free(symbol);
  return rank;
}

GameRuleMove read_move_rule(FILE *file) {
  char *symbol = read_value(file);
  GameRuleMove move = MOVE_ONE;
  if (strcmp(symbol, "any") == 0) {
    move = MOVE_ANY;
  } else if (strcmp(symbol, "group") == 0) {
    move = MOVE_GROUP;
  } else if (strcmp(symbol, "one") == 0) {
    move = MOVE_ONE;
  }
  free(symbol);
  return move;
}

GameRuleType read_from_rule(FILE *file) {
  char *symbol = read_value(file);
  GameRuleType type = RULE_ANY;
  if (strcmp(symbol, "foundation") == 0) {
    type = RULE_FOUNDATION;
  } else if (strcmp(symbol, "cell") == 0) {
    type = RULE_CELL;
  } else if (strcmp(symbol, "tableau") == 0) {
    type = RULE_TABLEAU;
  } else if (strcmp(symbol, "stock") == 0) {
    type = RULE_STOCK;
  } else if (strcmp(symbol, "waste") == 0) {
    type = RULE_WASTE;
  } else if (strcmp(symbol, "any") == 0) {
    type = RULE_ANY;
  }
  free(symbol);
  return type;
}

GameRule *define_game_rule(FILE *file, GameRuleType type, int index) {
  Keyword command;
  GameRule *rule = new_game_rule(type);
  begin_block(file);
  while ((command = read_command(file, game_rule_commands))) {
    switch (command) {
      case K_X:
        rule->x = read_expr(file, index);
        break;
      case K_Y:
        rule->y = read_expr(file, index);
        break;
      case K_DEAL:
        rule->deal = read_expr(file, index);
        break;
      case K_REDEAL:
        rule->redeals = read_expr(file, index);
        break;
      case K_HIDE:
        rule->hide = read_expr(file, index);
        break;
      case K_FIRST_RANK:
        rule->first_rank = read_rank(file);
        break;
      case K_FIRST_SUIT:
        rule->first_suit = read_suit(file);
        break;
      case K_NEXT_RANK:
        rule->next_rank = read_rank(file);
        break;
      case K_NEXT_SUIT:
        rule->next_suit = read_suit(file);
        break;
      case K_MOVE_GROUP:
        rule->move_group = read_move_rule(file);
        break;
      case K_FROM:
        rule->from = read_from_rule(file);
        break;
      case K_WIN_RANK:
        rule->win_rank = read_rank(file);
        break;
      case K_CLASS:
        rule->class = read_expr(file, index);
        break;
      case K_SAME_CLASS:
        rule->same_class = define_game_rule(file, type, index);
      default:
        break;
    }
  }
  end_block(file);
  return rule;
}

void execute_rule_block(FILE *file, Game *game, int index) {
  Keyword command;
  int i, rep;
  struct position pos;
  GameRule *rule = NULL;
  begin_block(file);
  while ((command = read_command(file, game_commands))) {
    switch (command) {
      case K_NAME:
        redefine_property(&game->name, file);
        break;
      case K_TITLE:
        redefine_property(&game->title, file);
        break;
      case K_REPEAT:
        rep = read_int(file);
        pos = get_position(file);
        for (i = 0; i < rep; i++) {
          set_position(pos, file);
          execute_rule_block(file, game, i);
        }
        break;
      case K_FOUNDATION:
        rule = define_game_rule(file, RULE_FOUNDATION, index);
        break;
      case K_TABLEAU:
        rule = define_game_rule(file, RULE_TABLEAU, index);
        break;
      case K_STOCK:
        rule = define_game_rule(file, RULE_STOCK, index);
        break;
      case K_CELL:
        rule = define_game_rule(file, RULE_CELL, index);
        break;
      case K_WASTE:
        rule = define_game_rule(file, RULE_WASTE, index);
        break;
      default:
        break;
    }
    if (rule) {
      if (game->last_rule) {
        game->last_rule->next = rule;
        game->last_rule = rule;
      } else {
        game->first_rule = rule;
        game->last_rule = rule;
      }
      rule = NULL;
    }
  }
  end_block(file);
}

void define_game(FILE *file) {
  Game *game = new_game();
  execute_rule_block(file, game, 0);
  register_game(game);
}

void set_property(const char *name, const char *value) {
  struct property *property = malloc(sizeof(struct property));
  size_t n1 = strlen(name);
  size_t n2 = strlen(value);
  property->name = calloc(n1 + 1, 1);
  memcpy(property->name, name, n1);
  property->value = calloc(n2 + 1, 1);
  memcpy(property->value, value, n2);
  property->next = properties;
  properties = property;
}

char *get_property(const char *name) {
  struct property *property;
  for (property = properties; property; property = property->next) {
    if (strcmp(property->name, name) == 0) {
      return property->value;
    }
  }
  return NULL;
}

int execute_file(const char *file_name) {
  Keyword command;
  int this_line, this_column;
  char *file_name_copy, *cwd, *value;
  FILE *file = fopen(file_name, "rb");
  if (!file) {
    rc_error("%s: error: %s", file_name, strerror(errno));
    return 1;
  }
  file_name_copy = strdup(file_name);
  cwd = dirname(file_name_copy);
  value = NULL;
  current_file = file_name;
  has_error = 0;
  line = 1;
  column = 1;
  while ((command = read_command(file, root_commands))) {
    switch (command) {
      case K_THEME:
        define_theme(file);
        break;
      case K_GAME:
        define_game(file);
        break;
      case K_INCLUDE:
        value = read_value(file);
        this_line = line;
        this_column = column;
        has_error |= !execute_file(value);
        line = this_line;
        column = this_column;
        current_file = file_name;
        free(value);
        break;
      case K_GAME_DIR:
        value = read_value(file);
        register_game_dir(cwd, value);
        free(value);
        break;
      case K_THEME_DIR:
        value = read_value(file);
        register_theme_dir(cwd, value);
        free(value);
        break;
      case K_DEFAULT_THEME:
        value = read_value(file);
        set_property("default_theme", value);
        free(value);
        break;
      case K_DEFAULT_GAME:
        value = read_value(file);
        set_property("default_game", value);
        free(value);
        break;
      case K_SCORES:
        scores_enabled = read_int(file);
        break;
      case K_SCORES_FILE:
        value = read_value(file);
        scores_file_path = combine_paths(cwd, value);
        free(value);
        break;
      case K_STATS:
        stats_enabled = read_int(file);
        break;
      case K_STATS_FILE:
        value = read_value(file);
        stats_file_path = combine_paths(cwd, value);
        free(value);
        break;
      case K_SHOW_SCORE:
        show_score = read_int(file);
        break;
      default:
        break;
    }
  }
  free(file_name_copy);
  fclose(file);
  return !has_error;
}

void execute_dir(const char *dir_path) {
  DIR *dir = opendir(dir_path);
  if (dir) {
    struct dirent *file;
    while ((file = readdir(dir))) {
      if (file->d_name[0] != '.') {
        char *path = combine_paths(dir_path, file->d_name);
        execute_file(path);
        free(path);
      }
    }
    closedir(dir);
  }
}
