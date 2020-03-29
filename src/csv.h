#ifndef CSV_H
#define CSV_G

#include <stdio.h>

/* column types:
 *  - s: char *
 *  - i: int32_t
 *  - t: time_t
 *  - *: ignore
 */
int read_csv(FILE *f, const char *columns, ...);

#endif
