all: moo.c
	$(CC) moo.c -o moo -Os

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f moo $(DESTDIR)$(PREFIX)/bin/moo
	chmod 755 $(DESTDIR)$(PREFIX)/bin/moo

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/moo
