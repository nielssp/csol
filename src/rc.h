/* csol
 * Copyright (c) 2017 Niels Sonnich Poulsen (http://nielssp.dk)
 * Licensed under the MIT license.
 * See the LICENSE file or http://opensource.org/licenses/MIT for more information.
 */

#ifndef RC_H
#define RC_H

struct color_name {
  char *symbol;
  int color;
};

extern struct color_name color_names[];

int execute_file(const char *file);

char *get_property(const char *name);

#endif
