OBJECTS = src/main.o src/config.o src/file.o src/buffer.o src/string.o src/kbdwidget.o \
	  src/window.o src/cmdbox.o src/editor.o src/vbox.o src/textview.o src/widget.o \
	  src/container.o src/multistring.o
OUTPUT = e
PHONY = clean install

CFLAGS = -Wall -pedantic -fPIC
LIBS = -lncurses -ltelex

ifeq ($(PREFIX), )
	PREFIX = /usr
endif

all: $(OUTPUT)

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -o root -g root -m 755 $(OUTPUT) $(DESTDIR)$(PREFIX)/bin/.

uninstall:
	rm -rf $(DESTDIR)$(PREFIX)/bin/$(OUTPUT)

clean:
	rm -rf $(OBJECTS) $(OUTPUT)

$(OUTPUT): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

.PHONY: $(PHONY)
