CC = gcc
CFLAGS = -O2 -Wall -std=c99
PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin
MANDIR = $(PREFIX)/share/man/man1
LICENSE_DIR = /usr/share/licenses/evap

all: evap

evap: src/evap.c
	$(CC) $(CFLAGS) -o evap src/evap.c

install: evap
	install -d $(DESTDIR)$(BINDIR)
	install -m 0755 evap $(DESTDIR)$(BINDIR)/evap
	install -d $(DESTDIR)$(MANDIR)
	install -m 0644 man/evap.1 $(DESTDIR)$(MANDIR)/evap.1
	install -d $(DESTDIR)$(LICENSE_DIR)
	install -m 0644 LICENSE $(DESTDIR)$(LICENSE_DIR)/LICENSE

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/evap
	rm -f $(DESTDIR)$(MANDIR)/evap.1
	rm -f $(DESTDIR)$(LICENSE_DIR)/LICENSE

clean:
	rm -f evap
