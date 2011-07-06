VERSION = $(shell git describe --tags --always --dirty)

CC = gcc
DESTDIR =
PREFIX = /usr/local
LIBS = -ldl
CFLAGS = -std=c99 -Wall -Wextra -pedantic -g -DVERSION=\"$(VERSION)\"
LDFLAGS = -g
SHARED_CFLAGS = -fPIC
SHARED_LDFLAGS = -shared -Wl,-soname,$@.$(MAJOR_VERSION)
