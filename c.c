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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <unistd.h>


int min(int x, int y) {
    return x > y ? y : x;
}


int levenshtein_distance(const char *s, const char *t) {
    int i, j;
    int sl = strlen(s);
    int tl = strlen(t);
    int d[sl+1][tl+1];

    for (i=0; i<=sl; i++)
        d[i][0] = i;

    for (j=0; j<=tl; j++)
        d[0][j] = j;

    for (j=1; j<=tl; j++) {
        for (i=1; i<=sl; i++) {
            if (s[i-1] == t[j-1]) {
                d[i][j] = d[i-1][j-1];
            }
            else {
                d[i][j] = min(d[i-1][j]+1, // deletion
                              min(d[i][j-1]+1, // insertion
                                  d[i-1][j-1]+1)); // substitution
            }
        }
    }
    return d[sl][tl];
}


int normalized_levenshtein_distance(const char *s, const char *t) {
    int sl = strlen(s);
    int tl = strlen(t);
    int d = levenshtein_distance(s,t);
    return 100 - (d * 100 / (sl > tl ? sl : tl));
}


struct dirent *aprox_dir_match(const char *path, const char *query, int threshold) {
    int i;
    DIR *dp;
    struct dirent *entry, *res = NULL;

    dp = opendir(path);
    while ((entry = readdir(dp))) {
        if (entry->d_type != DT_DIR)
            continue;
        i = normalized_levenshtein_distance(entry->d_name, query);
        if (i >= threshold) {
            res = entry;
            threshold = i;
            if (i == 100)
                break;
        }
    }
    closedir(dp);
    return res;
}


int main(int argc, const char *argv[]) {
    int path_max;
    char *token, *path, *query;
    struct dirent *dir;

    if (argc == 1) {
        printf("%s\n", getenv("HOME"));
        return 0;
    }

    if (strcmp(argv[1], "-") == 0) {
        printf(argv[1]);
        return 0;
    }

    query = malloc(strlen(argv[1]) * sizeof(*query));
    query = strcpy(query, argv[1]);

    path_max = pathconf("/", _PC_PATH_MAX);
    path = malloc(path_max * sizeof(*path));
    if (query[0] != '/')
        path = getcwd(path, path_max);
    strcat(path,"/");

    token = strtok(query, "/");
    if (!token) {
        printf("%s\n", path);
        return 0;
    }

    dir = aprox_dir_match(path, token, 50);
    while (dir) {
        strcat(path, dir->d_name);
        strcat(path, "/");
        token = strtok(NULL, "/");
        if (!token)
            break;
        dir = aprox_dir_match(path, token, 50);
    }

    if (dir)
        printf("%s\n", realpath(path,NULL));
    else
        printf("%s\n", argv[1]);

    return 0;
}
