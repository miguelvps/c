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


#include <ctype.h>
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

        s = MATCHER(dir->d_name, tokens->items[level]);
        if (s > THRESHOLD) {
            if (level + 1 >= tokens->size) {
                p = malloc(strlen(path) + strlen(dir->d_name) + 1);
                sprintf(p, "%s%s", path, dir->d_name);
                darray_append(result, entry_new(p, (score + s) / (level + 1)));
            }
            else {
                p = malloc(strlen(path) + strlen(dir->d_name) + 2);
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


    p = malloc((strlen(path) + 1) * sizeof(*p));
    p = strcpy(p, path);

    darray_init(&tokens, 10);
    token = strtok(p, "/");
    while (token != NULL) {
        darray_append(&tokens, token);
        token = strtok(NULL, "/");
    }

    entries = malloc(sizeof(*entries));
    darray_init(entries, 10);
    aprox_path_match_rec((path[0] == '/') ? "/" : "", &tokens, 0, 0, entries);

    darray_free(&tokens);
    free(p);

    return entries;
}

int print_dir_complete(const char *path, const char *starts_with, int full) {
    int i;
    char *p;
    DIR *dp;
    struct dirent *dir;

    i = 0;
    dp = opendir(path[0] == '\0' ? "." : path);
    while ((dir = readdir(dp))) {
        if (dir->d_type != DT_DIR || (starts_with[0] != '.'
                                      && (strcmp(dir->d_name, ".") == 0
                                          || strcmp(dir->d_name, "..") == 0)))
            continue;
        if ((p=strstr(dir->d_name, starts_with)) != NULL && p == dir->d_name) {
            if (full)
                printf("%s%s%s\n", path, path[0] == '/' ? "" : "/", dir->d_name);
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
    p = c = malloc((path_len + 1) * sizeof(*p));
    p = strcpy(p, path);
    dname = strrchr(p, '/');
    if (dname == p) {
        p = "/";
        dname++;
    }
    else if (dname != NULL) {
        (*dname) = '\0';
        dname++;
    }
    else {
        if (print_dir_complete("", p, 0))
            return;
    }
    if (stat(p, &buf) == 0 && S_ISDIR(buf.st_mode)) {
        if (print_dir_complete(p, dname, 1))
            return;
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
    int complete_flag;
    struct darray_entry *array;
    struct stat buf;

    complete_flag = 0;
    while ((i=getopt(argc, argv, "c")) != -1) {
        switch (i) {
        case 'c':
            complete_flag = 1;
            break;
        default:
            fprintf(stderr, "Usage: %s [-c] [directory]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (complete_flag) {
        /* argv[optind] = program name */
        /* argv[optind + 1] = str to be completed */
        /* prints each match followed by \n */
        complete(argv[optind + 1]);
        return 0;
    }

    if (optind >= argc) {
        printf("%s", getenv("HOME"));
        return 0;
    }

    if (strcmp(argv[optind], "-") == 0) {
        printf("%s", argv[optind]);
        return 0;
    }

    if (stat(argv[optind], &buf) == 0 && S_ISDIR(buf.st_mode)) {
        printf("%s", argv[optind]);
        return 0;
    }

    array = aprox_path_match(argv[optind]);
    if (array->size) {
        qsort(array->items, array->size, sizeof(*array->items), entry_compare);
        path = realpath(array->items[0]->dir, NULL);
        printf("%s", path);
        fprintf(stderr, "%.0f%% %s\n", array->items[0]->score * 100, path);
        free(path);
    } else
        printf("%s", argv[optind]);

    darray_destroy(array, entry_free, i);
    free(array);

    return 0;
}
