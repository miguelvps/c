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

#define _BSD_SOURCE

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "options.h"
#include "util.h"

#define darray(name, type) struct name { type *items; int size; int alloc; }

#define darray_init(a, n) do { \
                              (a)->items = s_malloc(n * sizeof(*(a)->items)); \
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
                                if ((a)->size >= (a)->alloc) \
                                    (a)->items = s_realloc((a)->items, ((a)->alloc*=2) * sizeof(*(a)->items)); \
                                (a)->items[(a)->size++] = i; \
                            } while (0)

darray(darray_entry, struct entry *);
darray(darray_string, char *);

struct entry {
    char *dir;
    double score;
};

struct entry *entry_new(char *dir, double score) {
    struct entry *entry = s_malloc(sizeof(*entry));
    entry->dir = dir;
    entry->score = score;
    return entry;
}

void entry_free(struct entry *entry) {
    free(entry->dir);
    free(entry);
}

int entry_compare(const void *a, const void *b) {
    struct entry **e1 = (struct entry **)a;
    struct entry **e2 = (struct entry **)b;
    return (int)(100.0 * (*e2)->score - 100.0 * (*e1)->score);
}

/* from path find match directories for tokens and return it in result */
void aprox_path_match_rec(const char *path, struct darray_string *tokens,
                          int level, double score,
                          struct darray_entry *result) {
    double s;
    char *p;
    DIR *dp;
    struct dirent *dir;

    dp = opendir(path[0] == '\0' ? "." : path);
    while ((dir = readdir(dp))) {
        if (dir->d_type != DT_DIR)
            continue;

        s = options.matcher(dir->d_name, tokens->items[level]);
        if (s > options.threshold) {
            if (level + 1 >= tokens->size) {
                p = s_malloc(strlen(path) + strlen(dir->d_name) + 1);
                sprintf(p, "%s%s", path, dir->d_name);
                darray_append(result, entry_new(p, (score + s) / (level + 1)));
            }
            else {
                p = s_malloc(strlen(path) + strlen(dir->d_name) + 2);
                sprintf(p, "%s%s/", path, dir->d_name);
                aprox_path_match_rec(p, tokens, level + 1, score + s, result);
                free(p);
            }
        }
    }
    closedir(dp);
}

struct darray_entry *aprox_path_match(const char *path) {
    char *p, *token;
    struct darray_string tokens;
    struct darray_entry *entries;


    p = s_malloc((strlen(path) + 1) * sizeof(*p));
    p = strcpy(p, path);

    darray_init(&tokens, 10);
    token = strtok(p, "/");
    while (token != NULL) {
        darray_append(&tokens, token);
        token = strtok(NULL, "/");
    }

    entries = s_malloc(sizeof(*entries));
    darray_init(entries, 10);
    aprox_path_match_rec((path[0] == '/') ? "/" : "", &tokens, 0, 0, entries);

    darray_free(&tokens);
    free(p);

    return entries;
}

int print_dir_complete(const char *path, const char *prefix, int full) {
    int i;
    DIR *dp;
    struct dirent *dir;

    i = 0;
    dp = opendir(path[0] == '\0' ? "." : path);
    while ((dir = readdir(dp))) {
        if (dir->d_type != DT_DIR || (prefix[0] != '.'
                                      && (strcmp(dir->d_name, ".") == 0
                                          || strcmp(dir->d_name, "..") == 0)))
            continue;
        if (str_starts_with(dir->d_name, prefix, options.icase)) {
            if (full)
                printf("%s%s%s\n", path,
                                   path[max(0, strlen(path) - 1)] == '/' ? "" : "/",
                                   dir->d_name);
            else
                printf("%s\n", dir->d_name);
            i++;
        }
    }
    closedir(dp);
    return i;
}

void complete(const char *path) {
    int i, path_len;
    char *p, *c, *dname;
    struct stat buf;
    struct darray_entry *entries;

    path_len = strlen(path);

    /* Normal completion */
    if (path_len == 0) {
        print_dir_complete("", "", 0);
        return;
    }
    p = c = s_malloc((path_len + 1) * sizeof(*p));
    p = strcpy(p, path);
    dname = strrchr(p, '/');
    if (dname == NULL) {
        if (print_dir_complete("", p, 0))
            return;
    }
    else {
        if (dname == p)
            p = "/";
        dname[0] = '\0';
        dname++;
        if (stat(p, &buf) == 0 && S_ISDIR(buf.st_mode)) {
            if (print_dir_complete(p, dname, 1))
                return;
        }
    }
    free(c);

    /* Aprox completion */
    entries = aprox_path_match(path);
    qsort(entries->items, entries->size, sizeof(*entries->items), entry_compare);
    if (path[path_len - 1] == '/')
        for (i = 0; i < entries->size; i++)
            print_dir_complete(entries->items[i]->dir, "", 1);
    else
        for (i = 0; i < entries->size; i++)
            printf("%s\n", entries->items[i]->dir);
    darray_destroy(entries, entry_free, i);
    free(entries);
}

int main(int argc, char * const argv[]) {
    int i;
    char *path;
    struct darray_entry *array;
    struct stat buf;

    parse_options(argc, argv);

    if (options.complete) {
        complete(options.directory);
        return 0;
    }

    if (!*options.directory) {
        printf("%s", getenv("HOME"));
        return 0;
    }

    if (strcmp(options.directory, "-") == 0) {
        printf("%s", options.directory);
        return 0;
    }

    if (stat(options.directory, &buf) == 0 && S_ISDIR(buf.st_mode)) {
        printf("%s", options.directory);
        return 0;
    }

    array = aprox_path_match(options.directory);
    if (array->size) {
        qsort(array->items, array->size, sizeof(*array->items), entry_compare);
        path = realpath(array->items[0]->dir, NULL);
        printf("%s", path);
        fprintf(stderr, "%.0f%% %s\n", array->items[0]->score * 100, path);
        free(path);
    } else
        printf("%s", options.directory);

    darray_destroy(array, entry_free, i);
    free(array);

    return 0;
}
