include config.mk

SRC = c.c jarowinkler.c
OBJ = $(SRC:.c=.o)

all: c

c: $(OBJ)
	$(CC) $(LDFLAGS) $(LIBS) -o $@ $(OBJ)

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f c $(OBJ)

install: all
	install -D -m 755 c $(DESTDIR)$(PREFIX)/bin/c
	install -D -m 755 c.sh $(DESTDIR)/etc/bash_completion.d/c

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/c
	rm -f $(DESTDIR)/etc/bash_completion.d/c

.PHONY: all clean install uninstall
