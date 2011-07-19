/* c: util.c
 * Copyright (C) 2011 Miguel Serrano
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <errno.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "options.h"

void error(int status, int errnum, const char *format, ...) {
    va_list args;

    fprintf(stderr, "%s: error: ", program_name);
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    if (errnum)
        fprintf(stderr, ": %s", strerror(errnum));
    fprintf(stderr, "\n");

    exit(status);
}

void *s_malloc(size_t size) {
    void *ptr;

    if (!(ptr = malloc(size)))
        error(EXIT_FAILURE, errno, "malloc");

    return ptr;
}

void *s_realloc(void *ptr, size_t size) {
    if (!(ptr = realloc(ptr, size)))
        error(EXIT_FAILURE, errno, "realloc");

    return ptr;
}

int str_starts_with(const char *str, const char *prefix, int icase) {
    if (icase)
        for (; *prefix && tolower(*str) == tolower(*prefix); str++, prefix++);
    else
        for (; *prefix && *str == *prefix; str++, prefix++);

    return !*prefix;
}

int max(int x, int y) {
    return x > y ? x : y;
}
