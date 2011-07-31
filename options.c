/* c: options.c
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

#define _XOPEN_SOURCE

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>

#include "options.h"
#include "util.h"

char *program_name;
struct options options;

void print_usage() {
    fprintf(stderr,
            "Usage: %s [-chisv] [-l library] [-m matcher] [-t threshold] [directory]\n",
            program_name);
}

void print_version() {
    fprintf(stderr, "c %s - a 'cd' wrapper\n", VERSION);
}

void parse_options(int argc, char *const argv[]) {
    int i;

    program_name = argv[0];

    memset(&options, 0, sizeof(options));
    options.matcher = MATCHER;
    options.threshold = THRESHOLD;
    options.directory = "";

    while ((i=getopt(argc, argv, "chisvl:m:t:")) != -1) {
        switch (i) {
            case 'c':
                options.complete = 1;
                break;
            case 'h':
                print_usage();
                exit(EXIT_SUCCESS);
            case 'i':
                options.icase = 1;
                break;
            case 'l':
                options.library = optarg;
                break;
            case 'm':
                options.matcher_name = optarg;
                break;
            case 's':
                options.simulate = 1;
                break;
            case 't':
                options.threshold = atof(optarg);
                break;
            case 'v':
                print_version();
                exit(EXIT_SUCCESS);
            default:
                print_usage();
                exit(EXIT_FAILURE);
        }
    }

    if (options.library && options.matcher_name) {
        void *handle;

        handle = dlopen(options.library, RTLD_LAZY);
        if (!handle)
            error(EXIT_FAILURE, 0, "dlopen: %s\n", dlerror());
        *(void **)(&options.matcher) = dlsym(handle, options.matcher_name);
        if (options.matcher == NULL)
            error(EXIT_FAILURE, 0, "dlsym: %s\n", dlerror());
    }

    if (options.complete && optind + 1 < argc)
        optind++;

    if (optind < argc)
        options.directory = argv[optind];
}
