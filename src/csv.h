/* csol
 * Copyright (c) 2020 Niels Sonnich Poulsen (http://nielssp.dk)
 * Licensed under the MIT license.
 * See the LICENSE file or http://opensource.org/licenses/MIT for more information.
 */

#ifndef CSV_H
#define CSV_H

#include <stdio.h>

/* column types:
 *  - s: char *
 *  - i: int32_t
 *  - t: time_t
 *  - *: ignore
 */
int read_csv(FILE *f, const char *columns, ...);

#endif
