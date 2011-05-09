/* c: c.c
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


#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "c.h"

#define darray(name, type) struct name { type *items; int size; int alloc; }

#define darray_init(a, n) do { \
                              (a)->items = malloc(n * sizeof(*(a)->items)); \
                                  if ((a)->items == NULL) { \
                                      perror("error: darray malloc"); \
                                      exit(EXIT_FAILURE); \
                                  } \
                              (a)->size = 0; \
                              (a)->alloc = n; \
                          } while (0)

#define darray_free(a) free((a)->items)

#define darray_destroy(a, d, i) do { \
                                    for (i=0; i<(a)->size; i++) \
                                        d((a)->items[i]); \
                                    darray_free(a); \
                                } while (0)

#define darray_append(a, i) do { \
                                if ((a)->size >= (a)->alloc) { \
                                    (a)->items = realloc((a)->items, ((a)->alloc*=2) * sizeof(*(a)->items)); \
                                    if ((a)->items == NULL) { \
                                        perror("error: darray realloc"); \
                                        exit(EXIT_FAILURE); \
                                    } \
                                } \
                                (a)->items[(a)->size++] = i; \
                            } while (0)

darray(darray_entry, struct entry *);
darray(darray_string, char *);

struct entry {
    char *dir;
    double score;
};

struct entry *entry_new(char *dir, double score) {
    struct entry *entry = malloc(sizeof(*entry));
    entry->dir = dir;
    entry->score = score;
    return entry;
}

void entry_free(struct entry *entry) {
    free(entry->dir);
    free(entry);
}

void aprox_path_match(const char *path, int level, double score,
                     struct darray_string *tokens, struct darray_entry *result) {
    double s;
    char *p;
    DIR *dp;
    struct dirent *dir;

    dp = opendir(path);
    while ((dir = readdir(dp))) {
        if (dir->d_type != DT_DIR || strcmp(dir->d_name, ".") == 0
                || strcmp(dir->d_name, "..") == 0)
            continue;

        s = MATCHER(dir->d_name, tokens->items[level]);
        if (s > THRESHOLD) {
            p = malloc(strlen(path) + strlen(dir->d_name) + 2);
            sprintf(p, "%s%s/", path, dir->d_name);
            if (level + 1 >= tokens->size)
                darray_append(result, entry_new(p, (score + s) / (level + 1)));
            else {
                aprox_path_match(p, level + 1, score + s, tokens, result);
                free(p);
            }
        }
    }
    closedir(dp);
}

int compare(const void *a, const void *b) {
    struct entry **e1 = (struct entry **)a;
    struct entry **e2 = (struct entry **)b;
    return (int)(100.0 * (*e2)->score - 100.0 * (*e1)->score);
}

int main(int argc, const char *argv[]) {
    int i;
    char *path, *token;
    struct darray_string tokens;
    struct darray_entry array;
    struct stat buf;

    if (argc <= 1) {
        printf("%s\n", getenv("HOME"));
        return 0;
    }

    if (strcmp(argv[1], "-") == 0) {
        printf(argv[1]);
        return 0;
    }

    if (stat(argv[1], &buf) == 0 && S_ISDIR(buf.st_mode)) {
        printf(argv[1]);
        return 0;
    }

    path = malloc((strlen(argv[1]) + 1) * sizeof(*path));
    path = strcpy(path, argv[1]);

    darray_init(&tokens, 10);
    token = strtok(path, "/");
    while (token != NULL) {
        darray_append(&tokens, token);
        token = strtok(NULL, "/");
    }

    darray_init(&array, 10);
    aprox_path_match((argv[1][0] == '/') ? "/" : "./", 0, 0, &tokens, &array);

    darray_free(&tokens);
    free(path);

    if (array.size) {
        qsort(array.items, array.size, sizeof(*array.items), compare);
        path = realpath(array.items[0]->dir, NULL);
        printf("%s\n", path);
        free(path);
    } else
        printf("%s\n", argv[1]);

    darray_destroy(&array, entry_free, i);

    return 0;
}
