#ifndef CSV_H
#define CSV_G

#include <stdio.h>

/* column types:
 *  - s: char *
 *  - i: int
 *  - t: time_t
 */
int read_csv(FILE *f, const char *columns, ...);

#endif
