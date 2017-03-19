#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>

#include "rc.h"
#include "theme.h"

const char *short_options = "hvlt:Tm";

const struct option long_options[] = {
  {"help", no_argument, NULL, 'h'},
  {"version", no_argument, NULL, 'v'},
  {"list", no_argument, NULL, 'l'},
  {"theme", required_argument, NULL, 't'},
  {"themes", no_argument, NULL, 'T'},
  {"mono", no_argument, NULL, 'm'},
  {0, 0, 0, 0}
};

enum action { PLAY, LIST_GAMES, LIST_THEMES };

void describe_option(const char *short_option, const char *long_option, const char *description) {
  printf("  -%-14s --%-18s %s\n", short_option, long_option, description);
}

int main(int argc, char *argv[]) {
  int opt;
  int option_index = 0;
  int colors = 1;
  enum action action = PLAY;
  char *game = NULL;
  char *theme = NULL;
  while ((opt = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
    switch (opt) {
      case 'h':
        printf("usage: %s [options] [game]\n", argv[0]);
        puts("options:");
        describe_option("h", "help", "Show help.");
        describe_option("v", "version", "Show version information.");
        describe_option("l", "list", "List available games.");
        describe_option("t <name>", "theme <name>", "Select a theme.");
        describe_option("T", "themes", "List available themes.");
        describe_option("m", "mono", "Disable colors.");
        return 0;
      case 'v':
        puts("csol 0.1.0");
        return 0;
      case 'l':
        action = LIST_GAMES;
        break;
      case 't':
        theme = optarg;
        break;
      case 'T':
        action = LIST_THEMES;
        break;
      case 'm':
        colors = 0;
        break;
    }
  }
  if (optind < argc) {
    game = argv[optind];
  }
  FILE *f = fopen("csolrc", "r");
  if (!f) {
    printf("csolrc: %s\n", strerror(errno));
  } else {
    execute_file(f);
    fclose(f);
  }
  switch (action) {
    case LIST_GAMES:
      printf("yukon - Yukon\n");
      break;
    case LIST_THEMES:
      for (ThemeList *list = list_themes(); list; list = list->next) {
        printf("%s - %s\n", list->theme->name, list->theme->title);
      }
      break;
    case PLAY:
      break;
  }
  return 0;
}
