CC = gcc
DESTDIR =
PREFIX = /usr/local
LIBS = -ldl
CFLAGS = -std=c99 -Wall -Wextra -pedantic -g
LDFLAGS = -g
SHARED_CFLAGS = -fPIC
SHARED_LDFLAGS = -shared -Wl,-soname,$@.$(MAJOR_VERSION)
