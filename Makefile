BIN=moo

$(BIN):
	$(CC) moo.c -o moo -Os

install: $(BIN)
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f moo $(DESTDIR)$(PREFIX)/bin/moo
	chmod 755 $(DESTDIR)$(PREFIX)/bin/moo

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/moo
