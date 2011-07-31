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

#define darray(type) \
    { type *items; int size; int alloc; }

#define darray_init(a, n) \
    do { \
        (a)->items = s_malloc(n * sizeof(*(a)->items)); \
        (a)->size = 0; \
        (a)->alloc = n; \
    } while (0)

#define darray_free(a) \
    free((a)->items)

#define darray_destroy(a, d, i) \
    do { \
        for (i=0; i<(a)->size; i++) \
        d((a)->items[i]); \
        darray_free(a); \
    } while (0)

#define darray_append(a, i) \
    do { \
        if ((a)->size >= (a)->alloc) \
        (a)->items = s_realloc((a)->items, \
                               ((a)->alloc*=2) * sizeof(*(a)->items)); \
        (a)->items[(a)->size++] = i; \
    } while (0)

struct darray_match darray(struct match *);
struct darray_string darray(char *);

struct match {
    char *dir;
    double score;
};

struct match *match_new(char *dir, double score) {
    struct match *match = s_malloc(sizeof(*match));

    match->dir = dir;
    match->score = score;
    return match;
}

void match_free(struct match *match) {
    free(match->dir);
    free(match);
}

int match_compare(const void *a, const void *b) {
    struct match **m1 = (struct match **)a;
    struct match **m2 = (struct match **)b;

    return ((*m1)->score > (*m2)->score) - ((*m1)->score < (*m2)->score);
}

/* from path find match directories for tokens and return it in matches */
void aprox_path_match_rec(const char *path, struct darray_string *tokens,
                          int level, double score,
                          struct darray_match *matches) {
    double s;
    char *p;
    DIR *dp;
    struct dirent *dir;

    dp = opendir(*path ? path : ".");
    if (dp == NULL)
        return;

    while ((dir = readdir(dp))) {
        if (dir->d_type != DT_DIR)
            continue;

        s = options.matcher(dir->d_name, tokens->items[level]);
        if (s > options.threshold) {
            if (level + 1 >= tokens->size) {
                p = s_malloc(strlen(path) + strlen(dir->d_name) + 1);
                sprintf(p, "%s%s", path, dir->d_name);
                darray_append(matches, match_new(p, (score + s) / (level + 1)));
            }
            else {
                p = s_malloc(strlen(path) + strlen(dir->d_name) + 2);
                sprintf(p, "%s%s/", path, dir->d_name);
                aprox_path_match_rec(p, tokens, level + 1, score + s, matches);
                free(p);
            }
        }
    }
    closedir(dp);
}

struct darray_match *aprox_path_match(const char *path) {
    char *p, *token;
    struct darray_string tokens;
    struct darray_match *matches;

    p = s_malloc((strlen(path) + 1) * sizeof(*p));
    p = strcpy(p, path);

    darray_init(&tokens, 10);
    token = strtok(p, "/");
    while (token != NULL) {
        darray_append(&tokens, token);
        token = strtok(NULL, "/");
    }

    matches = s_malloc(sizeof(*matches));
    darray_init(matches, 10);
    aprox_path_match_rec((path[0] == '/') ? "/" : "", &tokens, 0, 0, matches);

    darray_free(&tokens);
    free(p);

    return matches;
}

int print_dir_complete(const char *path, const char *prefix, int full) {
    int i;
    DIR *dp;
    struct dirent *dir;

    dp = opendir(*path ? path : ".");
    if (dp == NULL)
        return 0;

    i = 0;
    while ((dir = readdir(dp))) {
        if (dir->d_type != DT_DIR
            || (prefix[0] != '.'
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
    struct darray_match *matches;

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
    matches = aprox_path_match(path);
    qsort(matches->items, matches->size, sizeof(*matches->items),
          match_compare);
    if (path[path_len - 1] == '/')
        for (i = 0; i < matches->size; i++)
            print_dir_complete(matches->items[i]->dir, "", 1);
    else
        for (i = 0; i < matches->size; i++)
            printf("%s\n", matches->items[i]->dir);
    darray_destroy(matches, match_free, i);
    free(matches);
}

int main(int argc, char *const argv[]) {
    int i;
    char *path;
    struct darray_match *matches;
    struct stat buf;

    parse_options(argc, argv);

    if (options.simulate) {
        matches = aprox_path_match(options.directory);
        qsort(matches->items, matches->size, sizeof(*matches->items),
              match_compare);
        for (i = matches->size - 1; i >= 0; i--) {
            path = realpath(matches->items[i]->dir, NULL);
            fprintf(stderr, "%.0f%% %s\n", matches->items[i]->score * 100, path);
            free(path);
        }
        darray_destroy(matches, match_free, i);
        free(matches);
        return 0;
    }

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

    matches = aprox_path_match(options.directory);
    if (matches->size) {
        qsort(matches->items, matches->size, sizeof(*matches->items),
              match_compare);
        path = realpath(matches->items[matches->size - 1]->dir, NULL);
        printf("%s", path);
        fprintf(stderr, "%.0f%% %s\n", matches->items[0]->score * 100, path);
        free(path);
    }
    else
        printf("%s", options.directory);

    darray_destroy(matches, match_free, i);
    free(matches);

    return 0;
}
