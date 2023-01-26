/* csol
 * Copyright (c) 2017 Niels Sonnich Poulsen (http://nielssp.dk)
 * Licensed under the MIT license.
 * See the LICENSE file or http://opensource.org/licenses/MIT for more information.
 */

#ifndef COLOR_H
#define COLOR_H

#include "theme.h"

#define COLOR_PAIR_BACKGROUND 1
#define COLOR_PAIR_EMPTY 2
#define COLOR_PAIR_BACK 3
#define COLOR_PAIR_RED 4
#define COLOR_PAIR_BLACK 5

void init_theme_colors(Theme *theme);
void ui_list_colors();

#endif

