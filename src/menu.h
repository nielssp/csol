/* csol
 * Copyright (c) 2017 Niels Sonnich Poulsen (http://nielssp.dk)
 * Licensed under the MIT license.
 * See the LICENSE file or http://opensource.org/licenses/MIT for more information.
 */

#ifndef MENU_H
#define MENU_H

typedef struct Menu Menu;

struct Menu {
  char *label;
  char *key;
  int action;
  void *data;
  Menu *submenu;
};

typedef struct {
  int click;
  int x;
  int y;
} MenuClick;

#define MENU_IS_CLOSED 0
#define MENU_IS_OPEN -1
#define ACTION_GAME -2
#define ACTION_THEME -3

extern Menu game_menu[];
extern Menu theme_menu[];

void ui_message(const char *format, ...);
int ui_confirm(const char *message);
void ui_box(int y, int x, int height, int width, int fill);
void open_menu(int mnemonic, Menu *menu, Menu **menu_selection);
int ui_menubar(Menu *menu, Menu **menu_selection, void **data, MenuClick *click);

#endif
