#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "theme.h"
#include "game.h"
#include "rc.h"

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
  K_FG,
  K_BG
} Keyword;

struct symbol {
  char *symbol;
  Keyword keyword;
} commands[] = {
  {"theme", K_THEME},
  {"game", K_GAME},
  {"name", K_NAME},
  {"title", K_TITLE},
  {"heart", K_HEART},
  {"diamond", K_DIAMOND},
  {"spade", K_SPADE},
  {"club", K_CLUB},
  {"width", K_WIDTH},
  {"height", K_HEIGHT},
  {"top", K_TOP},
  {"middle", K_MIDDLE},
  {"bottom", K_BOTTOM},
  {"empty", K_EMPTY},
  {"back", K_BACK},
  {"red", K_RED},
  {"black", K_BLACK},
  {"fg", K_FG},
  {"bg", K_BG},
  {NULL, K_UNDEFINED}
};

void skip_whitespace(FILE *file) {
  int c;
  while (isspace(c = fgetc(file))) { }
  ungetc(c, file);
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
  int c = fgetc(file);
  if (!isalpha(c)) {
    ungetc(c, file);
    return NULL;
  }
  buffer = malloc(size);
  while (isalnum(c)) {
    buffer[i++] = c;
    if (i >= size) {
      buffer = resize_buffer(buffer, size, size + BUFFER_INC);
      size = size + BUFFER_INC;
    }
    c = fgetc(file);
  }
  ungetc(c, file);
  buffer[i] = '\0';
  return buffer;
}

char *read_quoted(FILE *file) {
  size_t size = BUFFER_INC, i = 0;
  int c = fgetc(file);
  char *buffer = malloc(size);
  while (c != EOF && c != '"') {
    buffer[i++] = c;
    if (i >= size) {
      buffer = resize_buffer(buffer, size, size + BUFFER_INC);
      size = size + BUFFER_INC;
    }
    c = fgetc(file);
  }
  buffer[i] = '\0';
  return buffer;
}

char *read_line(FILE *file) {
  size_t size = BUFFER_INC, i = 0;
  int c = fgetc(file);
  char *buffer = malloc(size);
  while (c != EOF && c != '\r' && c != '\n') {
    buffer[i++] = c;
    if (i >= size) {
      buffer = resize_buffer(buffer, size, size + BUFFER_INC);
      size = size + BUFFER_INC;
    }
    c = fgetc(file);
  }
  if (c == '\r') {
    c = fgetc(file);
    if (c != '\n') {
      ungetc(c, file);
    }
  }
  buffer[i] = '\0';
  return buffer;
}

char *read_value(FILE *file) {
  skip_whitespace(file);
  int c = fgetc(file);
  if (c == '"') {
    return read_quoted(file);
  }
  ungetc(c, file);
  return read_line(file);
}

int read_int(FILE *file) {
  char *str = read_value(file);
  int value = atoi(str);
  free(str);
  return value;
}

Keyword read_command(FILE *file) {
  skip_whitespace(file);
  char *keyword = read_symbol(file);
  if (!keyword) {
    return K_END_OF_BLOCK;
  }
  struct symbol *current = commands;
  while (current->symbol) {
    if (strcmp(current->symbol, keyword) == 0) {
      free(keyword);
      return current->keyword;
    }
    current++;
  }
  printf("undefined keyword: %s\n", keyword);
  free(keyword);
  return K_UNDEFINED;
}

int expect(int expected, FILE *file) {
  int actual = fgetc(file);
  if (actual != expected) {
    printf("unexpected '%c', expected '%c'\n", actual, expected);
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

Layout define_layout(FILE *file) {
  Keyword command;
  Layout layout;
  begin_block(file);
  while (command = read_command(file)) {
    switch (command) {
      case K_TOP:
        layout.top = read_value(file);
        break;
      case K_MIDDLE:
        layout.middle = read_value(file);
        break;
      case K_BOTTOM:
        layout.bottom = read_value(file);
        break;
      case K_FG:
        layout.color.fg = read_int(file);
        break;
      case K_BG:
        layout.color.bg = read_int(file);
        break;
    }
  }
  end_block(file);
  return layout;
}

void define_theme(FILE *file) {
  Keyword command;
  Theme *theme = malloc(sizeof(Theme));
  begin_block(file);
  while (command = read_command(file)) {
    switch (command) {
      case K_NAME:
        theme->name = read_value(file);
        break;
      case K_TITLE:
        theme->title = read_value(file);
        break;
      case K_HEART:
        theme->heart = read_value(file);
        break;
      case K_DIAMOND:
        theme->diamond = read_value(file);
        break;
      case K_SPADE:
        theme->spade = read_value(file);
        break;
      case K_CLUB:
        theme->club = read_value(file);
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
    }
  }
  end_block(file);
  register_theme(theme);
}

void define_game(FILE *file) {
  Keyword command;
  Game *game = malloc(sizeof(Game));
  begin_block(file);
  while (command = read_command(file)) {
    switch (command) {
      case K_NAME:
        game->name = read_value(file);
        break;
      case K_TITLE:
        game->title = read_value(file);
        break;
    }
  }
  end_block(file);
  register_game(game);
}


void execute_file(FILE *file) {
  Keyword command;
  while (command = read_command(file)) {
    switch (command) {
      case K_THEME:
        define_theme(file);
        break;
      case K_GAME:
        define_game(file);
        break;
    }
  }
}
