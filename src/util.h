/* csol
 * Copyright (c) 2017 Niels Sonnich Poulsen (http://nielssp.dk)
 * Licensed under the MIT license.
 * See the LICENSE file or http://opensource.org/licenses/MIT for more information.
 */

#ifndef UTIL_H
#define UTIL_H

#if defined(MSDOS) || defined(_WIN32)
#define PATH_SEP '\\'
#else
#define PATH_SEP '/'
#endif

#if !defined(USE_XDG_PATHS) && !defined(NO_XDG_PATHS)
#if defined(__unix__) || defined(__UNIX__) || defined(__linux__) || defined(__LINUX__)
#define USE_XDG_PATHS
#endif
#endif

int file_exists(const char *path);
char *combine_paths(const char *path1, const char *path2);
int mkdir_rec(const char *path);

#endif
