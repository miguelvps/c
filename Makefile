include config.mk

SRC = c.c jarowinkler.c options.c util.c
OBJ = $(SRC:.c=.o)

all: c c.1

.c.o:
	$(CC) $(CFLAGS) -c $<

c: $(OBJ)
	$(CC) $(LDFLAGS) $(LIBS) -o $@ $(OBJ)

c.1: README.pod
	pod2man --section=1 --name="C" --center="User Commands" --release="c ${VERSION}" $< > $@

clean:
	rm -f c c.1 $(OBJ)

install: all
	install -D c $(DESTDIR)$(PREFIX)/bin/c
	install -D c.sh $(DESTDIR)/etc/bash_completion.d/c
	install -D -m 644 c.1 $(DESTDIR)$(PREFIX)/share/man/man1/c.1

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/c
	rm -f $(DESTDIR)/etc/bash_completion.d/c
	rm -f $(DESTDIR)$(PREFIX)/share/man/man1/c.1

.PHONY: all clean install uninstall
