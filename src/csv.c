#include "csv.h"

#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>

static char *read_string(FILE *file) {
  int c;
  size_t size = 32, i = 0;
  char *buffer = malloc(size);
  while ((c = fgetc(file)) != EOF && c != ',' && c != '\n') {
    buffer[i++] = c;
    if (i >= size) {
      char *new_buffer;
      size += 32;
      new_buffer = realloc(buffer, size);
      if (!new_buffer) {
        i--;
        break;
      }
      buffer = new_buffer;
    }
  }
  buffer[i] = '\0';
  ungetc(c, file);
  return buffer;
}

static int read_int(FILE *file) {
  int c;
  int sign, value;
  while (isspace(c = fgetc(file))) { }
  ungetc(c, file);
  c = fgetc(file);
  sign = 1;
  value = 0;
  if (c == '-') {
    sign = -1;
    c = fgetc(file);
  }
  if (!isdigit(c)) {
    ungetc(c, file);
    return 0;
  }
  while (isdigit(c)) {
    value = value * 10 + c - '0';
    c = fgetc(file);
  }
  ungetc(c, file);
  return value * sign;
}

static time_t read_time(FILE *file) {
  time_t t = time(NULL);
  struct tm *time = gmtime(&t);
  if (fscanf(file, "%d-%d-%dT%d:%d:%dZ", &time->tm_year, &time->tm_mon, &time->tm_mday, &time->tm_hour,
        &time->tm_min, &time->tm_sec)) {
    time->tm_mon--;
    t = mktime(time);
    return t + (mktime(localtime(&t)) - mktime(gmtime(&t)));
  }
  return 0;
}

int read_csv(FILE *file, const char *columns, ...) {
  int c, eol = 0;
  va_list ap;
  while ((c = fgetc(file)) == '\n') { }
  if (c == EOF) {
    return 0;
  }
  ungetc(c, file);
  va_start(ap, columns);
  while (*columns) {
    switch (*(columns++)) {
      case 's': {
        char **s = va_arg(ap, char **);
        *s = eol ? NULL : read_string(file);
        break;
      }
      case 'i': {
        int *i = va_arg(ap, int *);
        *i = eol ? 0 : read_int(file);
        break;
      }
      case 't': {
        time_t *t = va_arg(ap, time_t *);
        *t = eol ? 0 : read_time(file);
        break;
      }
    }
    while ((c = fgetc(file)) != EOF && c != '\n' && c != ',') { }
    if (c == '\n' || c == EOF) {
      eol = 1;
    }
  }
  va_end(ap);
  if (!eol) {
  while (!feof(file) && fgetc(file) != '\n') { }
  }
  return 1;
}
