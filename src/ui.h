/* csol
 * Copyright (c) 2017 Niels Sonnich Poulsen (http://nielssp.dk)
 * Licensed under the MIT license.
 * See the LICENSE file or http://opensource.org/licenses/MIT for more information.
 */

#ifndef UI_H
#define UI_H

#include "game.h"
#include "theme.h"

void format_time(char *out, int32_t time);
void ui_main(Game *game, Theme *theme, int enable_color, unsigned int seed);
void ui_list_colors();

#endif
