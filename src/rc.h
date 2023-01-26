/* csol
 * Copyright (c) 2017 Niels Sonnich Poulsen (http://nielssp.dk)
 * Licensed under the MIT license.
 * See the LICENSE file or http://opensource.org/licenses/MIT for more information.
 */

#ifndef RC_H
#define RC_H

extern int smart_cursor;
extern int keep_vertical_position;
extern int alt_cursor;
extern int show_score;
extern int show_menu;

int execute_file(const char *file);
void execute_dir(const char *dir);

char *get_property(const char *name);

#endif
