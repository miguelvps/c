include ../config.mk

MAJOR_VERSION = 1
MINOR_VERSION = 0

SRC = levenshtein.c
OBJ = $(SRC:.c=.o)

all: liblevenshtein.so

liblevenshtein.so: $(OBJ)
	$(CC) $(LDFLAGS) $(SHARED_LDFLAGS) -o $@.$(MAJOR_VERSION).$(MINOR_VERSION) $(OBJ)
	ln -sf $@.$(MAJOR_VERSION).$(MINOR_VERSION) $@.$(MAJOR_VERSION)
	ln -sf $@.$(MAJOR_VERSION).$(MINOR_VERSION) $@

.c.o:
	$(CC) $(CFLAGS) $(SHARED_CFLAGS) -c $<

clean:
	rm -f liblevenshtein.so* $(OBJ)

install: all
	install -D liblevenshtein.so.$(MAJOR_VERSION).$(MINOR_VERSION) $(DESTDIR)$(PREFIX)/lib/liblevenshtein.so.$(MAJOR_VERSION).$(MINOR_VERSION)
	ldconfig -v -n $(DESTDIR)$(PREFIX)/lib
	ln -sf liblevenshtein.so.$(MAJOR_VERSION).$(MINOR_VERSION) $(DESTDIR)$(PREFIX)/lib/liblevenshtein.so

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/lib/liblevenshtein.so
	rm -f $(DESTDIR)$(PREFIX)/lib/liblevenshtein.so.$(MAJOR_VERSION)
	rm -f $(DESTDIR)$(PREFIX)/lib/liblevenshtein.so.$(MAJOR_VERSION).$(MINOR_VERSION)

.PHONY: all clean install uninstall
