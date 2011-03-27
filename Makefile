CC?=gcc
CFLAGS=-Wall -Wextra -g

SRCFILES=$(wildcard *.c)
OBJFILES=$(SRCFILES:.c=.o)

c: $(OBJFILES)

clean:
	rm -f c $(OBJFILES)
